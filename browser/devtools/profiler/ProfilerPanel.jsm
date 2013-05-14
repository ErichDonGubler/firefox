/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const Cu = Components.utils;

Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource:///modules/devtools/ProfilerController.jsm");
Cu.import("resource:///modules/devtools/ProfilerHelpers.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");

this.EXPORTED_SYMBOLS = ["ProfilerPanel"];

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
    "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

/**
 * An instance of a profile UI. Profile UI consists of
 * an iframe with Cleopatra loaded in it and some
 * surrounding meta-data (such as uids).
 *
 * Its main function is to talk to the Cleopatra instance
 * inside of the iframe.
 *
 * ProfileUI is also an event emitter. It emits the following events:
 *  - ready, when Cleopatra is done loading (you can also check the isReady
 *    property to see if a particular instance has been loaded yet.
 *  - disabled, when Cleopatra gets disabled. Happens when another ProfileUI
 *    instance starts the profiler.
 *  - enabled, when Cleopatra gets enabled. Happens when another ProfileUI
 *    instance stops the profiler.
 *
 * @param number uid
 *   Unique ID for this profile.
 * @param ProfilerPanel panel
 *   A reference to the container panel.
 */
function ProfileUI(uid, name, panel) {
  let doc = panel.document;
  let win = panel.window;

  EventEmitter.decorate(this);

  this.isReady = false;
  this.isStarted = false;
  this.isFinished = false;

  this.messages = [];
  this.panel = panel;
  this.uid = uid;
  this.name = name;

  this.iframe = doc.createElement("iframe");
  this.iframe.setAttribute("flex", "1");
  this.iframe.setAttribute("id", "profiler-cleo-" + uid);
  this.iframe.setAttribute("src", "cleopatra.html?" + uid);
  this.iframe.setAttribute("hidden", "true");

  // Append our iframe and subscribe to postMessage events.
  // They'll tell us when the underlying page is done loading
  // or when user clicks on start/stop buttons.

  doc.getElementById("profiler-report").appendChild(this.iframe);
  win.addEventListener("message", function (event) {
    if (parseInt(event.data.uid, 10) !== parseInt(this.uid, 10)) {
      return;
    }

    switch (event.data.status) {
      case "loaded":
        this.isReady = true;
        this.emit("ready");
        break;
      case "start":
        this.start();
        break;
      case "stop":
        this.stop();
        break;
      case "disabled":
        this.emit("disabled");
        break;
      case "enabled":
        this.emit("enabled");
        break;
      case "displaysource":
        this.panel.displaySource(event.data.data);
    }
  }.bind(this));
}

ProfileUI.prototype = {
  /**
   * Returns a contentWindow of the iframe pointing to Cleopatra
   * if it exists and can be accessed. Otherwise returns null.
   */
  get contentWindow() {
    if (!this.iframe) {
      return null;
    }

    try {
      return this.iframe.contentWindow;
    } catch (err) {
      return null;
    }
  },

  show: function PUI_show() {
    this.iframe.removeAttribute("hidden");
  },

  hide: function PUI_hide() {
    this.iframe.setAttribute("hidden", true);
  },

  /**
   * Send raw profiling data to Cleopatra for parsing.
   *
   * @param object data
   *   Raw profiling data from the SPS Profiler.
   * @param function onParsed
   *   A callback to be called when Cleopatra finishes
   *   parsing and displaying results.
   *
   */
  parse: function PUI_parse(data, onParsed) {
    if (!this.isReady) {
      return void this.on("ready", this.parse.bind(this, data, onParsed));
    }

    this.message({ task: "receiveProfileData", rawProfile: data }).then(() => {
      let poll = () => {
        let wait = this.panel.window.setTimeout.bind(null, poll, 100);
        let trail = this.contentWindow.gBreadcrumbTrail;

        if (!trail) {
          return wait();
        }

        if (!trail._breadcrumbs || !trail._breadcrumbs.length) {
          return wait();
        }

        onParsed();
      };

      poll();
    });
  },

  /**
   * Update profile's label in the sidebar.
   *
   * @param string text
   *   New text for the label.
   */
  updateLabel: function PUI_udpateLabel(text) {
    let doc = this.panel.document;
    let label = doc.querySelector("li#profile-" + this.uid + "> h1");
    label.textContent = text;
  },

  /**
   * Start profiling and, once started, notify the underlying page
   * so that it could update the UI. Also, once started, we add a
   * star to the profile name to indicate which profile is currently
   * running.
   *
   * @param function startFn
   *        A function to use instead of the default
   *        this.panel.startProfiling. Useful when you
   *        need mark panel as started after the profiler
   *        has been started elsewhere. It must take two
   *        params and call the second one.
   */
  start: function PUI_start(startFn) {
    if (this.isStarted || this.isFinished) {
      return;
    }

    startFn = startFn || this.panel.startProfiling.bind(this.panel);
    startFn(this.name, () => {
      this.isStarted = true;
      this.updateLabel(this.name + " *");
      this.panel.broadcast(this.uid, {task: "onStarted"}); // Do we really need this?
      this.emit("started");
    });
  },

  /**
   * Stop profiling and, once stopped, notify the underlying page so
   * that it could update the UI and remove a star from the profile
   * name.
   *
   * @param function stopFn
   *        A function to use instead of the default
   *        this.panel.stopProfiling. Useful when you
   *        need mark panel as stopped after the profiler
   *        has been stopped elsewhere. It must take two
   *        params and call the second one.
   */
  stop: function PUI_stop(stopFn) {
    if (!this.isStarted || this.isFinished) {
      return;
    }

    stopFn = stopFn || this.panel.stopProfiling.bind(this.panel);
    stopFn(this.name, () => {
      this.isStarted = false;
      this.isFinished = true;
      this.updateLabel(this.name);
      this.panel.broadcast(this.uid, {task: "onStopped"});
      this.emit("stopped");
    });
  },

  /**
   * Send a message to Cleopatra instance. If a message cannot be
   * sent, this method queues it for later.
   *
   * @param object data JSON data to send (must be serializable)
   * @return promise
   */
  message: function PIU_message(data) {
    let deferred = Promise.defer();
    let win = this.contentWindow;
    data = JSON.stringify(data);

    if (win) {
      win.postMessage(data, "*");
      deferred.resolve();
    } else {
      this.messages.push({ data: data, onSuccess: () => deferred.resolve() });
    }

    return deferred.promise;
  },

  /**
   * Send all queued messages (see this.message for more info)
   */
  flushMessages: function PIU_flushMessages() {
    if (!this.contentWindow) {
      return;
    }

    let msg;
    while (msg = this.messages.shift()) {
      this.contentWindow.postMessage(msg.data, "*");
      msg.onSuccess();
    }
  },

  /**
   * Destroys the ProfileUI instance.
   */
  destroy: function PUI_destroy() {
    this.isReady = null
    this.panel = null;
    this.uid = null;
    this.iframe = null;
    this.messages = null;
  }
};

/**
 * Profiler panel. It is responsible for creating and managing
 * different profile instances (see ProfileUI).
 *
 * ProfilerPanel is an event emitter. It can emit the following
 * events:
 *
 *   - ready:     after the panel is done loading everything,
 *                including the default profile instance.
 *   - started:   after the panel successfuly starts our SPS
 *                profiler.
 *   - stopped:   after the panel successfuly stops our SPS
 *                profiler and is ready to hand over profiling
 *                data
 *   - parsed:    after Cleopatra finishes parsing profiling
 *                data.
 *   - destroyed: after the panel cleans up after itself and
 *                is ready to be destroyed.
 *
 * The following events are used mainly by tests to prevent
 * accidential oranges:
 *
 *   - profileCreated:  after a new profile is created.
 *   - profileSwitched: after user switches to a different
 *                      profile.
 */
function ProfilerPanel(frame, toolbox) {
  this.isReady = false;
  this.window = frame.window;
  this.document = frame.document;
  this.target = toolbox.target;

  this.profiles = new Map();
  this._uid = 0;
  this._msgQueue = {};

  EventEmitter.decorate(this);
}

ProfilerPanel.prototype = {
  isReady:     null,
  window:      null,
  document:    null,
  target:      null,
  controller:  null,
  profiles:    null,

  _uid:        null,
  _activeUid:  null,
  _runningUid: null,
  _browserWin: null,
  _msgQueue:   null,

  get activeProfile() {
    return this.profiles.get(this._activeUid);
  },

  set activeProfile(profile) {
    this._activeUid = profile.uid;
  },

  get browserWindow() {
    if (this._browserWin) {
      return this._browserWin;
    }

    let win = this.window.top;
    let type = win.document.documentElement.getAttribute("windowtype");

    if (type !== "navigator:browser") {
      win = Services.wm.getMostRecentWindow("navigator:browser");
    }

    return this._browserWin = win;
  },

  /**
   * Open a debug connection and, on success, switch to the newly created
   * profile.
   *
   * @return Promise
   */
  open: function PP_open() {
    let promise;

    // Local profiling needs to make the target remote.
    if (!this.target.isRemote) {
      promise = this.target.makeRemote();
    } else {
      promise = Promise.resolve(this.target);
    }

    return promise
      .then(function(target) {
        let deferred = Promise.defer();
        this.controller = new ProfilerController(this.target);

        this.controller.connect(function onConnect() {
          let create = this.document.getElementById("profiler-create");
          create.addEventListener("click", function (ev) {
            this.createProfile()
          }.bind(this), false);
          create.removeAttribute("disabled");

          let profile = this.createProfile();
          this.switchToProfile(profile, function () {
            this.isReady = true;
            this.emit("ready");

            deferred.resolve(this);
          }.bind(this))
        }.bind(this));

        return deferred.promise;
      }.bind(this))
      .then(null, function onError(reason) {
        Cu.reportError("ProfilerPanel open failed. " +
                       reason.error + ": " + reason.message);
      });
  },

  /**
   * Creates a new profile instance (see ProfileUI) and
   * adds an appropriate item to the sidebar. Note that
   * this method doesn't automatically switch user to
   * the newly created profile, they have do to switch
   * explicitly.
   *
   * @param string name
   *        (optional) name of the new profile
   *
   * @return ProfilerPanel
   */
  createProfile: function PP_createProfile(name) {
    if (name && this.getProfileByName(name)) {
      return this.getProfileByName(name);
    }

    let uid = ++this._uid;

    // If profile is anonymous, increase its UID until we get
    // to the unused name. This way if someone manually creates
    // a profile named say 'Profile 2' we won't create a dup
    // with the same name. We will just skip over uid 2.

    if (!name) {
      name = L10N.getFormatStr("profiler.profileName", [uid]);
      while (this.getProfileByName(name)) {
        uid = ++this._uid;
        name = L10N.getFormatStr("profiler.profileName", [uid]);
      }
    }

    let list = this.document.getElementById("profiles-list");
    let item = this.document.createElement("li");
    let wrap = this.document.createElement("h1");
    name = name || L10N.getFormatStr("profiler.profileName", [uid]);

    item.setAttribute("id", "profile-" + uid);
    item.setAttribute("data-uid", uid);
    item.addEventListener("click", function (ev) {
      this.switchToProfile(this.profiles.get(uid));
    }.bind(this), false);

    wrap.className = "profile-name";
    wrap.textContent = name;

    item.appendChild(wrap);
    list.appendChild(item);

    let profile = new ProfileUI(uid, name, this);
    this.profiles.set(uid, profile);

    this.emit("profileCreated", uid);
    return profile;
  },

  /**
   * Switches to a different profile by making its instance an
   * active one.
   *
   * @param ProfileUI profile
   *   A profile instance to switch to.
   * @param function onLoad
   *   A function to call when profile instance is ready.
   *   If the instance is already loaded, onLoad will be
   *   called synchronously.
   */
  switchToProfile: function PP_switchToProfile(profile, onLoad=function() {}) {
    let doc = this.document;

    if (this.activeProfile) {
      this.activeProfile.hide();
    }

    let active = doc.querySelector("#profiles-list > li.splitview-active");
    if (active) {
      active.className = "";
    }

    doc.getElementById("profile-" + profile.uid).className = "splitview-active";
    profile.show();
    this.activeProfile = profile;

    if (profile.isReady) {
      profile.flushMessages();
      this.emit("profileSwitched", profile.uid);
      onLoad();
      return;
    }

    profile.once("ready", () => {
      profile.flushMessages();
      this.emit("profileSwitched", profile.uid);
      onLoad();
    });
  },

  /**
   * Start collecting profile data.
   *
   * @param function onStart
   *   A function to call once we get the message
   *   that profiling had been successfuly started.
   */
  startProfiling: function PP_startProfiling(name, onStart) {
    this.controller.start(name, (err) => {
      if (err) {
        return void Cu.reportError("ProfilerController.start: " + err.message);
      }

      onStart();
      this.emit("started");
    });
  },

  /**
   * Stop collecting profile data and send it to Cleopatra
   * for parsing.
   *
   * @param function onStop
   *   A function to call once we get the message
   *   that profiling had been successfuly stopped.
   */
  stopProfiling: function PP_stopProfiling(name, onStop) {
    this.controller.isActive(function (err, isActive) {
      if (err) {
        Cu.reportError("ProfilerController.isActive: " + err.message);
        return;
      }

      if (!isActive) {
        return;
      }

      this.controller.stop(name, function (err, data) {
        if (err) {
          Cu.reportError("ProfilerController.stop: " + err.message);
          return;
        }

        this.activeProfile.data = data;
        this.activeProfile.parse(data, function onParsed() {
          this.emit("parsed");
        }.bind(this));

        onStop();
        this.emit("stopped", data);
      }.bind(this));
    }.bind(this));
  },

  /**
   * Lookup an individual profile by its name.
   *
   * @param string name name of the profile
   * @return profile object or null
   */
  getProfileByName: function PP_getProfileByName(name) {
    if (!this.profiles) {
      return null;
    }

    for (let [ uid, profile ] of this.profiles) {
      if (profile.name === name) {
        return profile;
      }
    }

    return null;
  },

  /**
   * Lookup an individual profile by its UID.
   *
   * @param number uid UID of the profile
   * @return profile object or null
   */
  getProfileByUID: function PP_getProfileByUID(uid) {
    if (!this.profiles) {
      return null;
    }

    return this.profiles.get(uid) || null;
  },

  /**
   * Iterates over each available profile and calls
   * a callback with it as a parameter.
   *
   * @param function cb a callback to call
   */
  eachProfile: function PP_eachProfile(cb) {
    let uid = this._uid;

    if (!this.profiles) {
      return;
    }

    while (uid >= 0) {
      if (this.profiles.has(uid)) {
        cb(this.profiles.get(uid));
      }

      uid -= 1;
    }
  },

  /**
   * Broadcast messages to all Cleopatra instances.
   *
   * @param number target
   *   UID of the recepient profile. All profiles will receive the message
   *   but the profile specified by 'target' will have a special property,
   *   isCurrent, set to true.
   * @param object data
   *   An object with a property 'task' that will be sent over to Cleopatra.
   */
  broadcast: function PP_broadcast(target, data) {
    if (!this.profiles) {
      return;
    }

    if (data.task === "onStarted") {
      this._runningUid = target;
    } else {
      this._runningUid = null;
    }

    this.eachProfile((profile) => {
      profile.message({
        uid: target,
        isCurrent: target === profile.uid,
        task: data.task
      });
    });
  },

  /**
   * Open file specified in data in either a debugger or view-source.
   *
   * @param object data
   *   An object describing the file. It must have three properties:
   *    - uri
   *    - line
   *    - isChrome (chrome files are opened via view-source)
   */
  displaySource: function PP_displaySource(data, onOpen=function() {}) {
    let win = this.window;
    let panelWin, timeout;

    function onSourceShown(event) {
      if (event.detail.url !== data.uri) {
        return;
      }

      panelWin.removeEventListener("Debugger:SourceShown", onSourceShown, false);
      panelWin.editor.setCaretPosition(data.line - 1);
      onOpen();
    }

    if (data.isChrome) {
      return void this.browserWindow.gViewSourceUtils.
        viewSource(data.uri, null, this.document, data.line);
    }

    gDevTools.showToolbox(this.target, "jsdebugger").then(function (toolbox) {
      let dbg = toolbox.getCurrentPanel();
      panelWin = dbg.panelWin;

      let view = dbg.panelWin.DebuggerView;
      if (view.Sources.selectedValue === data.uri) {
        view.editor.setCaretPosition(data.line - 1);
        onOpen();
        return;
      }

      panelWin.addEventListener("Debugger:SourceShown", onSourceShown, false);
      panelWin.DebuggerView.Sources.preferredSource = data.uri;
    }.bind(this));
  },

  /**
   * Cleanup.
   */
  destroy: function PP_destroy() {
    if (this.profiles) {
      let uid = this._uid;

      while (uid >= 0) {
        if (this.profiles.has(uid)) {
          this.profiles.get(uid).destroy();
          this.profiles.delete(uid);
        }
        uid -= 1;
      }
    }

    if (this.controller) {
      this.controller.destroy();
    }

    this.isReady = null;
    this.window = null;
    this.document = null;
    this.target = null;
    this.controller = null;
    this.profiles = null;
    this._uid = null;
    this._activeUid = null;

    this.emit("destroyed");
  }
};
