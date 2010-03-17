// -*- Mode: js2; tab-width: 2; indent-tabs-mode: nil; js2-basic-offset: 2; js2-skip-preprocessor-directives: t; -*-
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Mobile Browser.
 *
 * The Initial Developer of the Original Code is Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mark Finkle <mfinkle@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const PREFIX_ITEM_URI = "urn:mozilla:item:";
const PREFIX_NS_EM = "http://www.mozilla.org/2004/em-rdf#";

const PREF_GETADDONS_REPOSITORY = "extensions.getAddons.repository";
const PREF_GETADDONS_MAXRESULTS = "extensions.getAddons.maxResults";

const URI_GENERIC_ICON_XPINSTALL = "chrome://browser/skin/images/alert-addons-30.png";

var ExtensionsView = {
  _extmgr: null,
  _pref: null,
  _rdf: null,
  _ios: null,
  _strings: {},
  _repo: null,
  _list: null,
  _localItem: null,
  _repoItem: null,
  _msg: null,
  _dloadmgr: null,
  _search: null,
  _restartCount: 0,
  _observerIndex: -1,

  _isXPInstallEnabled: function ev__isXPInstallEnabled() {
    let enabled = false;
    let locked = false;
    try {
      enabled = this._pref.getBoolPref("xpinstall.enabled");
      if (enabled)
        return true;
      locked = this._pref.prefIsLocked("xpinstall.enabled");
    }
    catch (e) { }

    return false;
  },

  _getIDFromURI: function ev__getIDFromURI(aURI) {
    if (aURI.substring(0, PREFIX_ITEM_URI.length) == PREFIX_ITEM_URI)
      return aURI.substring(PREFIX_ITEM_URI.length);
    return aURI;
  },

  _getRDFProperty: function ev__getRDFProperty(aID, aName) {
    let resource = this._rdf.GetResource(PREFIX_ITEM_URI + aID);
    if (resource) {
      let ds = this._extmgr.datasource;

      let target = ds.GetTarget(resource, this._rdf.GetResource(PREFIX_NS_EM + aName), true);
      if (target && target instanceof Ci.nsIRDFLiteral)
        return target.Value;
    }
    return null;
  },

  _createItem: function ev__createItem(aAddon, aTypeName) {
    let item = document.createElement("richlistitem");
    item.setAttribute("id", PREFIX_ITEM_URI + aAddon.id);
    item.setAttribute("addonID", aAddon.id);
    item.setAttribute("typeName", aTypeName);
    item.setAttribute("type", aAddon.type);
    item.setAttribute("typeLabel", this._strings["addonType." + aAddon.type]);
    item.setAttribute("name", aAddon.name);
    item.setAttribute("version", aAddon.version);
    item.setAttribute("iconURL", aAddon.iconURL);
    return item;
  },

  clearSection: function ev_clearSection(aSection) {
    let start = null;
    let end = null;

    if (aSection == "local") {
      start = this._localItem;
      end = this._repoItem;
    }
    else {
      start = this._repoItem;
    }

    while (start.nextSibling != end)
      this._list.removeChild(start.nextSibling);
  },

  _messageActions: function ev__messageActions(aData) {
    if (aData == "addons-restart-app") {
      // Notify all windows that an application quit has been requested
      var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
      var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      os.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      // If nothing aborted, quit the app
      if (cancelQuit.data == false) {
        let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
        appStartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
      }
    }
  },

  getElementForAddon: function ev_getElementForAddon(aID) {
    let element = document.getElementById(PREFIX_ITEM_URI + aID);
    if (!element && this._list)
      element = this._list.getElementsByAttribute("xpiURL", aID)[0];
    return element;
  },

  showMessage: function ev_showMessage(aMsg, aValue, aButtonLabel, aShowCloseButton, aNotifyData) {
    let notification = this._msg.getNotificationWithValue(aValue);
    if (notification)
      return;

    let self = this;
    let buttons = null;
    if (aButtonLabel) {
      buttons = [ {
        label: aButtonLabel,
        accessKey: "",
        data: aNotifyData,
        callback: function(aNotification, aButton) {
          self._messageActions(aButton.data);
          return true;
        }
      } ];
    }

    this._msg.appendNotification(aMsg, aValue, "", this._msg.PRIORITY_WARNING_LOW, buttons).hideclose = !aShowCloseButton;
  },

  showRestart: function ev_showRestart() {
    // Increment the count in case the view is not completely initialized
    this._restartCount++;

    if (this._msg) {
      let strings = Elements.browserBundle;
      this.showMessage(strings.getString("notificationRestart.label"), "restart-app",
                       strings.getString("notificationRestart.button"), false, "addons-restart-app");
    }
  },

  hideRestart: function ev_hideRestart() {
    this._restartCount--;
    if (this._restartCount == 0 && this._msg) {
      let notification = this._msg.getNotificationWithValue("restart-app");
      if (notification)
        notification.close();
    }
  },

  showOptions: function ev_showOptions(aID) {
    this.hideOptions();

    let item = this.getElementForAddon(aID);
    if (!item)
      return;

    // if the element is not the selected element, select it
    if (item != this._list.selectedItem)
      this._list.selectedItem = item;

    item.showOptions();
  },

  hideOptions: function ev_hideOptions() {
    if (!this._list)
      return;

    let items = this._list.childNodes;
    for (let i = 0; i < items.length; i++) {
      let item = items[i];
      if (item.hideOptions)
        item.hideOptions();
    }
    this._list.ensureSelectedElementIsVisible();
  },

  get visible() {
    let items = document.getElementById("panel-items");
    if (BrowserUI.isPanelVisible() && items.selectedPanel.id == "addons-container")
      return true;
    return false;
  },

  init: function ev_init() {
    if (this._extmgr)
      return;

    this._extmgr = Cc["@mozilla.org/extensions/manager;1"].getService(Ci.nsIExtensionManager);
    this._ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    this._dloadmgr = new XPInstallDownloadManager();
    this._observerIndex = this._extmgr.addInstallListener(this._dloadmgr);

    // Now look and see if we're being opened by XPInstall
    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.addObserver(this._dloadmgr, "xpinstall-download-started", false);

    // Watch for add-on update notifications
    os.addObserver(this, "addon-update-started", false);
    os.addObserver(this, "addon-update-ended", false);

    let self = this;
    let panels = document.getElementById("panel-items");
    panels.addEventListener("select",
                            function(aEvent) {
                              if (panels.selectedPanel.id == "addons-container")
                                self._delayedInit();
                            },
                            false);
  },

  _delayedInit: function ev__delayedInit() {
    if (this._list)
      return;

    this._pref = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch2);
    this._rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
    this._search = Cc["@mozilla.org/browser/search-service;1"].getService(Ci.nsIBrowserSearchService);

    let repository = "@mozilla.org/extensions/addon-repository;1";
    try {
      var repo = pref.getCharPref(PREF_GETADDONS_REPOSITORY);
      if (repo in Components.classes)
        repository = repo;
    } catch (e) { }
    this._repo = Cc[repository].createInstance(Ci.nsIAddonRepository);

    this._list = document.getElementById("addons-list");
    this._localItem = document.getElementById("addons-local");
    this._repoItem = document.getElementById("addons-repo");
    this._msg = document.getElementById("addons-messages");

    // Show the restart notification in case a restart is needed, but the view
    // was not visible at the time
    let notification = this._msg.getNotificationWithValue("restart-app");
    if (this._restartCount > 0 && !notification) {
      this.showRestart();
      this._restartCount--; // showRestart() always increments
    }

    let strings = Elements.browserBundle;
    this._strings["addonType.2"] = strings.getString("addonType.2");
    this._strings["addonType.4"] = strings.getString("addonType.4");
    this._strings["addonType.8"] = strings.getString("addonType.8");
    this._strings["addonType.1024"] = strings.getString("addonType.1024");

    let self = this;
    setTimeout(function() {
      self.getAddonsFromLocal();
      self.getAddonsFromRepo("");
    }, 0);
  },

  uninit: function ev_uninit() {
    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.removeObserver(this._dloadmgr, "xpinstall-download-started");
    os.removeObserver(this, "addon-update-started");
    os.removeObserver(this, "addon-update-ended");

    this._extmgr.removeInstallListenerAt(this._observerIndex);
  },

  hideOnSelect: function ev_handleEvent(aEvent) {
    // When list selection changes, be sure to close up any open options sections
    if (aEvent.target == this._list)
      this.hideOptions();
  },

  getAddonsFromLocal: function ev_getAddonsFromLocal() {
    this.clearSection("local");

    let items = this._extmgr.getItemList(Ci.nsIUpdateItem.TYPE_ANY, {});

    for (let i = 0; i < items.length; i++) {
      let addon = items[i];

      // Some information is not directly accessible from the extmgr
      let isDisabled = this._getRDFProperty(addon.id, "isDisabled") == "true";
      let appDisabled = this._getRDFProperty(addon.id, "appDisabled");
      let appManaged = this._getRDFProperty(addon.id, "appManaged");
      let desc = this._getRDFProperty(addon.id, "description");
      let optionsURL = this._getRDFProperty(addon.id, "optionsURL");
      let opType = this._getRDFProperty(addon.id, "opType");
      let updateable = this._getRDFProperty(addon.id, "updateable");

      let listitem = this._createItem(addon, "local");
      listitem.setAttribute("isDisabled", isDisabled);
      listitem.setAttribute("appDisabled", appDisabled);
      listitem.setAttribute("appManaged", appManaged);
      listitem.setAttribute("description", desc);
      listitem.setAttribute("optionsURL", optionsURL);
      listitem.setAttribute("opType", opType);
      listitem.setAttribute("updateable", updateable);
      this._list.insertBefore(listitem, this._repoItem);
    }

    // Load the search engines
    let defaults = this._search.getDefaultEngines({ }).map(function (e) e.name);
    function isDefault(aEngine)
      defaults.indexOf(aEngine.name) != -1

    let strings = Elements.browserBundle;
    let defaultDescription = strings.getString("addonsSearchEngine.description");

    let engines = this._search.getEngines({ });
    for (let e = 0; e < engines.length; e++) {
      let engine = engines[e];
      let addon = {};
      addon.id = engine.name;
      addon.type = 1024;
      addon.name = engine.name;
      addon.version = "";
      addon.iconURL = engine.iconURI ? engine.iconURI.spec : "";

      let listitem = this._createItem(addon, "searchplugin");
      listitem._engine = engine;
      listitem.setAttribute("isDisabled", engine.hidden ? "true" : "false");
      listitem.setAttribute("appDisabled", "false");
      listitem.setAttribute("appManaged", isDefault(engine));
      listitem.setAttribute("description", engine.description || defaultDescription);
      listitem.setAttribute("optionsURL", "");
      listitem.setAttribute("opType", engine.hidden ? "needs-disable" : "");
      listitem.setAttribute("updateable", "false");
      this._list.insertBefore(listitem, this._repoItem);
    }

    if (engines.length + items.length == 0) {
      this.displaySectionMessage("local", strings.getString("addonsLocalNone.label"), null, true);
      document.getElementById("addons-update-all").disabled = true;
    }
  },

  enable: function ev_enable(aItem) {
    let opType;
    if (aItem.getAttribute("type") == "1024") {
      aItem._engine.hidden = false;
      opType = "needs-enable";
    } else {
      let id = this._getIDFromURI(aItem.id);
      this._extmgr.enableItem(id);
      opType = this._getRDFProperty(id, "opType");

      if (opType == "needs-enable")
        this.showRestart();
      else
        this.hideRestart();
    }

    aItem.setAttribute("opType", opType);
  },

  disable: function ev_disable(aItem) {
    let opType;
    if (aItem.getAttribute("type") == "1024") {
      aItem._engine.hidden = true;
      opType = "needs-disable";
    } else {
      let id = this._getIDFromURI(aItem.id);
      this._extmgr.disableItem(id);
      opType = this._getRDFProperty(id, "opType");

      if (opType == "needs-disable")
        this.showRestart();
      else
        this.hideRestart();
    }

    aItem.setAttribute("opType", opType);
  },

  uninstall: function ev_uninstall(aItem) {
    let opType;
    if (aItem.getAttribute("type") == "1024") {
      // Make sure the engine isn't hidden before removing it, to make sure it's
      // visible if the user later re-adds it (works around bug 341833)
      aItem._engine.hidden = false;
      this._search.removeEngine(aItem._engine);
      // the search-engine-modified observer in browser.js will take care of
      // updating the list
    } else {
      let id = this._getIDFromURI(aItem.id);
      this._extmgr.uninstallItem(id);
      opType = this._getRDFProperty(id, "opType");

      if (opType == "needs-uninstall")
        this.showRestart();
      aItem.setAttribute("opType", opType);
    }
  },

  cancelUninstall: function ev_cancelUninstall(aItem) {
    let id = this._getIDFromURI(aItem.id);
    this._extmgr.cancelUninstallItem(id);

    this.hideRestart();

    let opType = this._getRDFProperty(id, "opType");
    aItem.setAttribute("opType", opType);
  },

  _installCallback: function ev__installCallback(aItem, aStatus) {
    if (aStatus == -210) {
      // User cancelled
      aItem.removeAttribute("opType");
    }
    else if (aStatus < 0) {
      // Some other error
      aItem.removeAttribute("opType");
      let bundles = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
      let strings = bundles.createBundle("chrome://global/locale/xpinstall/xpinstall.properties");

      try {
        var msg = strings.GetStringFromName("error" + aStatus);
      } catch (ex) {
        msg = strings.formatStringFromName("unknown.error", [aStatus]);
      }
      aItem.setAttribute("error", msg);
    }
    else {
      // Success
      aItem.setAttribute("opType", "needs-restart");
    }
  },

  installFromRepo: function ev_installFromRepo(aItem) {
    if (!this._isXPInstallEnabled())
      return;

    if (aItem.hasAttribute("eula")) {
      var eula = {
        name: aSelectedItem.getAttribute("name"),
        text: aSelectedItem.getAttribute("eula"),
        accepted: false
      };

      // TODO: eula
      //window.openDialog("chrome://mozapps/content/extensions/eula.xul", "_blank",
      //                  "chrome,dialog,modal,centerscreen,resizable=no", eula);
      //if (!eula.accepted)
      //  return;
    }

    var details = {
      URL: aItem.getAttribute("xpiURL"),
      Hash: aItem.getAttribute("xpiHash"),
      IconURL: aItem.getAttribute("iconURL"),
      toString: function () { return this.URL; }
    };

    var params = [];
    params[aItem.getAttribute("name")] = details;

    let self = this;
    InstallTrigger.install(params, function(aURL, aStatus) { self._installCallback(aItem, aStatus); });
  },

  installFromXPI: function ev_installAddons(aItems, aManager) {
    this._extmgr.addDownloads(aItems, aItems.length, aManager);
  },

  _isSafeURI: function ev_isSafeURI(aURL) {
    try {
      var uri = this._ios.newURI(aURL, null, null);
      var scheme = uri.scheme;
    } catch (ex) {}
    return (uri && (scheme == "http" || scheme == "https" || scheme == "ftp"));
  },

  displaySectionMessage: function ev_displaySectionMessage(aSection, aMessage, aButtonLabel, aHideThrobber) {
    let item = document.createElement("richlistitem");
    item.setAttribute("typeName", "message");
    item.setAttribute("message", aMessage);
    if (aButtonLabel)
      item.setAttribute("button", aButtonLabel);
    else
      item.setAttribute("hidebutton", "true");
    item.setAttribute("hidethrobber", aHideThrobber);

    if (aSection == "repo")
      this._list.appendChild(item);
    else
      this._list.insertBefore(item, this._repoItem);

    return item;
  },

  getAddonsFromRepo: function ev_getAddonsFromRepo(aTerms, aSelectFirstResult) {
    this.clearSection("repo");

    // Make sure we're online before attempting to load
    Util.forceOnline();

    if (this._repo.isSearching)
      this._repo.cancelSearch();

    let strings = Elements.browserBundle;
    if (aTerms) {
      AddonSearchResults.selectFirstResult = aSelectFirstResult;
      this.displaySectionMessage("repo", strings.getString("addonsSearchStart.label"),
                                strings.getString("addonsSearchStart.button"), false);
      this._repo.searchAddons(aTerms, this._pref.getIntPref(PREF_GETADDONS_MAXRESULTS), AddonSearchResults);
    }
    else {
      if (RecommendedSearchResults.cache) {
        this.displaySearchResults(RecommendedSearchResults.cache, -1, true);
      }
      else {
        this.displaySectionMessage("repo", strings.getString("addonsSearchStart.label"),
                                  strings.getString("addonsSearchStart.button"), false);
        this._repo.retrieveRecommendedAddons(this._pref.getIntPref(PREF_GETADDONS_MAXRESULTS), RecommendedSearchResults);
      }
    }
  },

  displaySearchResults: function ev_displaySearchResults(aAddons, aTotalResults, aIsRecommended, aSelectFirstResult) {
    this.clearSection("repo");

    let strings = Elements.browserBundle;
    if (aAddons.length == 0) {
      let msg = aIsRecommended ? strings.getString("addonsSearchNone.recommended") :
                                 strings.getString("addonsSearchNone.search");
      let button = aIsRecommended ? strings.getString("addonsSearchNone.button") :
                                    strings.getString("addonsSearchSuccess2.button");
      let item = this.displaySectionMessage("repo", msg, button, true);

      if (aSelectFirstResult)
        this._list.scrollBoxObject.scrollToElement(item);
      return;
    }

    if (aIsRecommended) {
      // Locale sensitive sort
      function compare(a, b) {
        return String.localeCompare(a.name, b.name);
      }
      aAddons.sort(compare);
    }

    var urlproperties = [ "iconURL", "homepageURL", "thumbnailURL", "xpiURL" ];
    var properties = [ "name", "eula", "iconURL", "homepageURL", "thumbnailURL", "xpiURL", "xpiHash" ];
    var foundItem = false;
    for (let i = 0; i < aAddons.length; i++) {
      let addon = aAddons[i];

      // Check for any items with potentially unsafe urls
      if (urlproperties.some(function (p) !this._isSafeURI(addon[p]), this))
        continue;

      let listitem = this._createItem(addon, "search");
      listitem.setAttribute("description", addon.summary);
      listitem.setAttribute("homepageURL", addon.homepageURL);
      listitem.setAttribute("xpiURL", addon.xpiURL);
      listitem.setAttribute("xpiHash", addon.xpiHash);
      if (!aIsRecommended)
        listitem.setAttribute("rating", addon.rating);
      let item = this._list.appendChild(listitem);

      if (aSelectFirstResult && !foundItem) {
        foundItem = true;
        this._list.selectItem(item);
        this._list.scrollBoxObject.scrollToElement(item);
      }
    }

    if (!aIsRecommended)
      this.displaySectionMessage("repo", null, strings.getString("addonsSearchSuccess2.button"), true);
  },

  showPage: function ev_showPage(aItem) {
    let uri = aItem.getAttribute("homepageURL");
    if (uri)
      BrowserUI.newTab(uri);
  },

  get searchBox() {
    delete this.searchBox;
    return this.searchBox = document.getElementById("addons-search-text");
  },

  doSearch: function ev_doSearch(aTerms) {
    this.searchBox.value = aTerms;
    this.getAddonsFromRepo(aTerms, true);
  },

  resetSearch: function ev_resetSearch() {
    this.searchBox.value = "";
    this.getAddonsFromRepo("");
  },

  observe: function ev_observe(aSubject, aTopic, aData) {
    if (!document)
      return;

    let addon = aSubject.QueryInterface(Ci.nsIUpdateItem);
    let strings = Elements.browserBundle;
    let element = document.getElementById(PREFIX_ITEM_URI + addon.id);
    if (!element)
      return;

    switch (aTopic) {
      case "addon-update-started":
        element.setAttribute("updateStatus", strings.getString("addonUpdate.checking"));
        break;
      case "addon-update-ended":
        let status = parseInt(aData);
        let updateable = false;
        const nsIAUCL = Ci.nsIAddonUpdateCheckListener;
        switch (status) {
          case nsIAUCL.STATUS_UPDATE:
            var statusMsg = strings.getFormattedString("addonUpdate.updating", [addon.version]);
            updateable = true;
            break;
          case nsIAUCL.STATUS_VERSIONINFO:
            statusMsg = strings.getString("addonUpdate.compatibility");
            break;
          case nsIAUCL.STATUS_FAILURE:
            statusMsg = strings.getString("addonUpdate.error");
            break;
          case nsIAUCL.STATUS_DISABLED:
            statusMsg = strings.getString("addonUpdate.disabled");
            break;
          case nsIAUCL.STATUS_APP_MANAGED:
          case nsIAUCL.STATUS_NO_UPDATE:
            statusMsg = strings.getString("addonUpdate.noupdate");
            break;
          case nsIAUCL.STATUS_NOT_MANAGED:
            statusMsg = strings.getString("addonUpdate.notsupported");
            break;
          case nsIAUCL.STATUS_READ_ONLY:
            statusMsg = strings.getString("addonUpdate.notsupported");
            break;
          default:
            statusMsg = strings.getString("addonUpdate.noupdate");
        }
        element.setAttribute("updateStatus", statusMsg);

        // Tag the add-on so the XPInstallDownloadManager knows it's an update
        if (updateable)
          element.setAttribute("updating", "true");
        break;
    }
  }
};


function searchFailed() {
  ExtensionsView.clearSection("repo");

  let strings = Elements.browserBundle;
  let brand = document.getElementById("bundle_brand");

  let failLabel = strings.getFormattedString("addonsSearchFail.label",
                                             [brand.getString("brandShortName")]);
  let failButton = strings.getString("addonsSearchFail.button");
  ExtensionsView.displaySectionMessage("repo", failLabel, failButton, true);
}


///////////////////////////////////////////////////////////////////////////////
// nsIAddonSearchResultsCallback for the recommended search
var RecommendedSearchResults = {
  cache: null,

  searchSucceeded: function(aAddons, aAddonCount, aTotalResults) {
    this.cache = aAddons;
    ExtensionsView.displaySearchResults(aAddons, aTotalResults, true);
  },

  searchFailed: searchFailed
}

///////////////////////////////////////////////////////////////////////////////
// nsIAddonSearchResultsCallback for a standard search
var AddonSearchResults = {
  // set by ExtensionsView
  selectFirstResult: false,

  searchSucceeded: function(aAddons, aAddonCount, aTotalResults) {
    ExtensionsView.displaySearchResults(aAddons, aTotalResults, false, this.selectFirstResult);
  },

  searchFailed: searchFailed
}

///////////////////////////////////////////////////////////////////////////////
// XPInstall download helper
function XPInstallDownloadManager() {
}

XPInstallDownloadManager.prototype = {
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpinstall-download-started":
        var params = aSubject.QueryInterface(Components.interfaces.nsISupportsArray);
        var paramBlock = params.GetElementAt(0).QueryInterface(Components.interfaces.nsISupportsInterfacePointer);
        paramBlock = paramBlock.data.QueryInterface(Components.interfaces.nsIDialogParamBlock);
        var manager = params.GetElementAt(1).QueryInterface(Components.interfaces.nsISupportsInterfacePointer);
        manager = manager.data.QueryInterface(Components.interfaces.nsIObserver);
        this.addDownloads(paramBlock, manager);
        break;
    }
  },

  addDownloads: function (aParams, aManager) {
    let count = aParams.GetInt(1);
    let items = [];
    for (var i = 0; i < count;) {
      let displayName = aParams.GetString(i++);
      let url = aParams.GetString(i++);
      let iconURL = aParams.GetString(i++);
      let uri = ExtensionsView._ios.newURI(url, null, null);
      let isTheme = uri.QueryInterface(Ci.nsIURL).fileExtension.toLowerCase() == "jar";
      let type = isTheme ? Ci.nsIUpdateItem.TYPE_THEME : Ci.nsIUpdateItem.TYPE_EXTENSION;
      if (!iconURL) {
        iconURL = isTheme ? "chrome://mozapps/skin/extensions/themeGeneric.png" :
                            "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
      }

      let item = Cc["@mozilla.org/updates/item;1"].createInstance(Ci.nsIUpdateItem);
      item.init(url, " ", "app-profile", "", "", displayName, url, "", iconURL, "", "", type, "");
      items.push(item);

      // Advance the enumerator
      let certName = aParams.GetString(i++);
    }

    this._failed = [];
    this._succeeded = [];

    ExtensionsView.installFromXPI(items, aManager);

    if (ExtensionsView.visible)
      return;

    let strings = Elements.browserBundle;
    var alerts = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    alerts.showAlertNotification(URI_GENERIC_ICON_XPINSTALL, strings.getString("alertAddons"),
                                 strings.getString("alertAddonsInstalling"), false, "", null);
  },

  /////////////////////////////////////////////////////////////////////////////
  // nsIAddonInstallListener
  onDownloadStarted: function(aAddon) { },
  onDownloadEnded: function(aAddon) { },
  onInstallStarted: function(aAddon) { },
  onCompatibilityCheckStarted: function(aAddon) { },
  onCompatibilityCheckEnded: function(aAddon, aStatus) { },

  _failed: [],
  _succeeded: [],
  onInstallEnded: function(aAddon, aStatus) {
    // Track success/failure for each addon
    if (Components.isSuccessCode(aStatus))
      this._succeeded.push(aAddon.id);
    else
      this._failed.push(aAddon.id);

    if (!ExtensionsView.visible)
      return;

    var element = ExtensionsView.getElementForAddon(aAddon.id);
    if (!element)
      return;

    element.setAttribute("status", (Components.isSuccessCode(aStatus) ? "success" : "fail"));

    // If we are updating an add-on, change the status
    if (element.hasAttribute("updating")) {
      let strings = Elements.browserBundle;
      element.setAttribute("updateStatus", strings.getFormattedString("addonUpdate.updated", [aAddon.version]));
      element.removeAttribute("updating");
    }
  },

  onInstallsCompleted: function() {
    let strings = Elements.browserBundle;

    // If even one add-on succeeded, display the restart notif
    if (this._succeeded.length > 0)
      ExtensionsView.showRestart();

    if (ExtensionsView.visible)
      return;

    let message = strings.getString("alertAddonsInstalled");
    if (this._succeeded.length == 0 && this._failed.length > 0)
      message = strings.getString("alertAddonsFail");

    let observer = {
      observe: function (aSubject, aTopic, aData) {
        if (aTopic == "alertclickcallback")
          BrowserUI.showPanel("addons-container");
      }
    };

    let alerts = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    alerts.showAlertNotification(URI_GENERIC_ICON_XPINSTALL, strings.getString("alertAddons"),
                                 message, true, "", observer);
  },

  onDownloadProgress: function xpidm_onDownloadProgress(aAddon, aValue, aMaxValue) {
    var element = ExtensionsView.getElementForAddon(aAddon.id);
    if (!element)
      return;

    let opType = element.getAttribute("opType");
    if (!opType) {
      element.setAttribute("opType", "needs-install");
    }
    var progress = Math.round((aValue / aMaxValue) * 100);
    element.setAttribute("progress", progress);
  },

  /////////////////////////////////////////////////////////////////////////////
  // nsISupports
  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsIAddonInstallListener) &&
        !aIID.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
