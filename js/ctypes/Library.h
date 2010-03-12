/* -*-  Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
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
 * The Original Code is js-ctypes.
 *
 * The Initial Developer of the Original Code is
 * The Mozilla Foundation <http://www.mozilla.org/>.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Mark Finkle <mark.finkle@gmail.com>, <mfinkle@mozilla.com>
 *  Dan Witte <dwitte@mozilla.com>
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

#ifndef LIBRARY_H
#define LIBRARY_H

struct PRLibrary;

namespace mozilla {
namespace ctypes {

class Function;

enum LibrarySlot {
  SLOT_LIBRARY = 0,
  SLOT_FUNCTIONLIST = 1,
  LIBRARY_SLOTS
};

class Library
{
public:
  static JSObject* Create(JSContext* cx, jsval aPath);
  static void Trace(JSTracer *trc, JSObject* obj);
  static void Finalize(JSContext* cx, JSObject* obj);

  static PRLibrary* GetLibrary(JSContext* cx, JSObject* obj);
  static bool AddFunction(JSContext* cx, JSObject* aLibrary, Function* aFunction);

  // JSFastNative functions
  static JSBool Open(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Close(JSContext* cx, uintN argc, jsval* vp);
  static JSBool Declare(JSContext* cx, uintN argc, jsval* vp);

private:
  // nothing to instantiate here!
  Library();
};

}
}

#endif
