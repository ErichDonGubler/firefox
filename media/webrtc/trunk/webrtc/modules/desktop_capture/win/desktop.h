/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DESKTOP_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DESKTOP_H_

#include <string>
#include <windows.h>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Desktop {
 public:
  ~Desktop();

  // Returns the name of the desktop represented by the object. Return false if
  // quering the name failed for any reason.
  bool GetName(std::wstring* desktop_name_out) const;

  // Returns true if |other| has the same name as this desktop. Returns false
  // in any other case including failing Win32 APIs and uninitialized desktop
  // handles.
  bool IsSame(const Desktop& other) const;

  // Assigns the desktop to the current thread. Returns false is the operation
  // failed for any reason.
  bool SetThreadDesktop() const;

  // Returns the desktop by its name or NULL if an error occurs.
  static Desktop* GetDesktop(const wchar_t* desktop_name);

  // Returns the desktop currently receiving user input or NULL if an error
  // occurs.
  static Desktop* GetInputDesktop();

  // Returns the desktop currently assigned to the calling thread or NULL if
  // an error occurs.
  static Desktop* GetThreadDesktop();

 private:
  Desktop(HDESK desktop, bool own);

  // The desktop handle.
  HDESK desktop_;

  // True if |desktop_| must be closed on teardown.
  bool own_;

  DISALLOW_COPY_AND_ASSIGN(Desktop);
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DESKTOP_H_
