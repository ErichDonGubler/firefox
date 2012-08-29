/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * This file contains code for synchronizing engines.
 */

const EXPORTED_SYMBOLS = ["EngineSynchronizer"];

const {utils: Cu} = Components;

Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/util.js");

/**
 * Perform synchronization of engines.
 *
 * This was originally split out of service.js. The API needs lots of love.
 */
function EngineSynchronizer(service) {
  this._log = Log4Moz.repository.getLogger("Sync.Synchronizer");
  this._log.level = Log4Moz.Level[Svc.Prefs.get("log.logger.synchronizer")];

  this.service = service;

  this.onComplete = null;
}

EngineSynchronizer.prototype = {
  sync: function sync() {
    if (!this.onComplete) {
      throw new Error("onComplete handler not installed.");
    }

    let startTime = Date.now();

    Status.resetSync();

    // Make sure we should sync or record why we shouldn't.
    let reason = this.service._checkSync();
    if (reason) {
      if (reason == kSyncNetworkOffline) {
        Status.sync = LOGIN_FAILED_NETWORK_ERROR;
      }

      // this is a purposeful abort rather than a failure, so don't set
      // any status bits
      reason = "Can't sync: " + reason;
      this.onComplete(new Error("Can't sync: " + reason));
      return;
    }

    // If we don't have a node, get one. If that fails, retry in 10 minutes.
    if (!this.service.clusterURL && !this.service._clusterManager.setCluster()) {
      Status.sync = NO_SYNC_NODE_FOUND;
      this._log.info("No cluster URL found. Cannot sync.");
      this.onComplete(null);
      return;
    }

    // Ping the server with a special info request once a day.
    let infoURL = this.service.infoURL;
    let now = Math.floor(Date.now() / 1000);
    let lastPing = Svc.Prefs.get("lastPing", 0);
    if (now - lastPing > 86400) { // 60 * 60 * 24
      infoURL += "?v=" + WEAVE_VERSION;
      Svc.Prefs.set("lastPing", now);
    }

    // Figure out what the last modified time is for each collection
    let info = this.service._fetchInfo(infoURL);

    // Convert the response to an object and read out the modified times
    for (let engine of [Clients].concat(this.service.engineManager.getAll())) {
      engine.lastModified = info.obj[engine.name] || 0;
    }

    if (!(this.service._remoteSetup(info))) {
      this.onComplete(new Error("Aborting sync, remote setup failed"));
      return;
    }

    // Make sure we have an up-to-date list of clients before sending commands
    this._log.debug("Refreshing client list.");
    if (!this._syncEngine(Clients)) {
      // Clients is an engine like any other; it can fail with a 401,
      // and we can elect to abort the sync.
      this._log.warn("Client engine sync failed. Aborting.");
      this.onComplete(null);
      return;
    }

    // Wipe data in the desired direction if necessary
    switch (Svc.Prefs.get("firstSync")) {
      case "resetClient":
        this.service.resetClient(this.service.enabledEngineNames);
        break;
      case "wipeClient":
        this.service.wipeClient(this.service.enabledEngineNames);
        break;
      case "wipeRemote":
        this.service.wipeRemote(this.service.enabledEngineNames);
        break;
    }

    if (Clients.localCommands) {
      try {
        if (!(Clients.processIncomingCommands())) {
          Status.sync = ABORT_SYNC_COMMAND;
          this.onComplete(new Error("Processed command aborted sync."));
          return;
        }

        // Repeat remoteSetup in-case the commands forced us to reset
        if (!(this.service._remoteSetup(info))) {
          this.onComplete(new Error("Remote setup failed after processing commands."));
          return;
        }
      }
      finally {
        // Always immediately attempt to push back the local client (now
        // without commands).
        // Note that we don't abort here; if there's a 401 because we've
        // been reassigned, we'll handle it around another engine.
        this._syncEngine(Clients);
      }
    }

    // Update engines because it might change what we sync.
    try {
      this._updateEnabledEngines();
    } catch (ex) {
      this._log.debug("Updating enabled engines failed: " +
                      Utils.exceptionStr(ex));
      ErrorHandler.checkServerError(ex);
      this.onComplete(ex);
      return;
    }

    try {
      for each (let engine in this.service.engineManager.getEnabled()) {
        // If there's any problems with syncing the engine, report the failure
        if (!(this._syncEngine(engine)) || Status.enforceBackoff) {
          this._log.info("Aborting sync for failure in " + engine.name);
          break;
        }
      }

      // If _syncEngine fails for a 401, we might not have a cluster URL here.
      // If that's the case, break out of this immediately, rather than
      // throwing an exception when trying to fetch metaURL.
      if (!this.service.clusterURL) {
        this._log.debug("Aborting sync, no cluster URL: " +
                        "not uploading new meta/global.");
        this.onComplete(null);
        return;
      }

      // Upload meta/global if any engines changed anything
      let meta = Records.get(this.service.metaURL);
      if (meta.isNew || meta.changed) {
        new Resource(this.service.metaURL).put(meta);
        delete meta.isNew;
        delete meta.changed;
      }

      // If there were no sync engine failures
      if (Status.service != SYNC_FAILED_PARTIAL) {
        Svc.Prefs.set("lastSync", new Date().toString());
        Status.sync = SYNC_SUCCEEDED;
      }
    } finally {
      Svc.Prefs.reset("firstSync");

      let syncTime = ((Date.now() - startTime) / 1000).toFixed(2);
      let dateStr = new Date().toLocaleFormat(LOG_DATE_FORMAT);
      this._log.info("Sync completed at " + dateStr
                     + " after " + syncTime + " secs.");
    }

    this.onComplete(null);
  },

  // Returns true if sync should proceed.
  // false / no return value means sync should be aborted.
  _syncEngine: function _syncEngine(engine) {
    try {
      engine.sync();
    }
    catch(e) {
      if (e.status == 401) {
        // Maybe a 401, cluster update perhaps needed?
        // We rely on ErrorHandler observing the sync failure notification to
        // schedule another sync and clear node assignment values.
        // Here we simply want to muffle the exception and return an
        // appropriate value.
        return false;
      }
    }

    return true;
  },

  _updateEnabledEngines: function _updateEnabledEngines() {
    this._log.info("Updating enabled engines: " + SyncScheduler.numClients + " clients.");
    let meta = Records.get(this.service.metaURL);
    if (meta.isNew || !meta.payload.engines)
      return;

    // If we're the only client, and no engines are marked as enabled,
    // thumb our noses at the server data: it can't be right.
    // Belt-and-suspenders approach to Bug 615926.
    if ((SyncScheduler.numClients <= 1) &&
        ([e for (e in meta.payload.engines) if (e != "clients")].length == 0)) {
      this._log.info("One client and no enabled engines: not touching local engine status.");
      return;
    }

    this.service._ignorePrefObserver = true;

    let enabled = this.service.enabledEngineNames;
    for (let engineName in meta.payload.engines) {
      if (engineName == "clients") {
        // Clients is special.
        continue;
      }
      let index = enabled.indexOf(engineName);
      if (index != -1) {
        // The engine is enabled locally. Nothing to do.
        enabled.splice(index, 1);
        continue;
      }
      let engine = this.service.engineManager.get(engineName);
      if (!engine) {
        // The engine doesn't exist locally. Nothing to do.
        continue;
      }

      if (Svc.Prefs.get("engineStatusChanged." + engine.prefName, false)) {
        // The engine was disabled locally. Wipe server data and
        // disable it everywhere.
        this._log.trace("Wiping data for " + engineName + " engine.");
        engine.wipeServer();
        delete meta.payload.engines[engineName];
        meta.changed = true;
      } else {
        // The engine was enabled remotely. Enable it locally.
        this._log.trace(engineName + " engine was enabled remotely.");
        engine.enabled = true;
      }
    }

    // Any remaining engines were either enabled locally or disabled remotely.
    for each (let engineName in enabled) {
      let engine = this.service.engineManager.get(engineName);
      if (Svc.Prefs.get("engineStatusChanged." + engine.prefName, false)) {
        this._log.trace("The " + engineName + " engine was enabled locally.");
      } else {
        this._log.trace("The " + engineName + " engine was disabled remotely.");
        engine.enabled = false;
      }
    }

    Svc.Prefs.resetBranch("engineStatusChanged.");
    this.service._ignorePrefObserver = false;
  },

};
Object.freeze(EngineSynchronizer.prototype);
