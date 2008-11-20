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
 * The Original Code is Weave.
 *
 * The Initial Developer of the Original Code is Mozilla.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Dan Mills <thunder@mozilla.com>
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

const EXPORTED_SYMBOLS = ['WBORecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

function WBORecord(uri, authenticator) {
  this._WBORec_init(uri, authenticator);
}
WBORecord.prototype = {
  __proto__: Resource.prototype,
  _logName: "Record.WBO",

  _WBORec_init: function WBORec_init(uri, authenticator) {
    this._init(uri, authenticator);
    this.pushFilter(new WBOFilter());
    this.pushFilter(new JsonFilter());
    this.data = {
      // modified: "2454725.98283", // FIXME
      payload: {}
    };
  },

  // id is special because it's based on the uri
  get id() {
    if (this.data.id)
      return decodeURI(this.data.id);
    let foo = this.uri.spec.split('/');
    return decodeURI(foo[foo.length-1]);
  },

  get parentid() this.data.parentid,
  set parentid(value) {
    this.data.parentid = value;
  },

  get modified() this.data.modified,
  set modified(value) {
    this.data.modified = value;
  },

  get encoding() this.data.encoding,
  set encoding(value) {
    this.data.encoding = value;
  },

  get payload() this.data.payload,
  set payload(value) {
    this.data.payload = value;
  }

  // note: encryption is part of the CryptoWrapper object, which uses
  // a string (encrypted) payload instead of an object
};

function WBOFilter() {}
WBOFilter.prototype = {
  beforePUT: function(data, wbo) {
    let self = yield;
    let foo = wbo.uri.spec.split('/');
    data.id = decodeURI(foo[foo.length-1]);
    self.done(data);
  },
  afterGET: function(data, wbo) {
    let self = yield;
    let foo = wbo.uri.spec.split('/');
    data.id = decodeURI(foo[foo.length-1]);
    self.done(data);
  }
};
