/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SHARED_DESKTOP_FRAME_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SHARED_DESKTOP_FRAME_H_

#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"

namespace webrtc {

// SharedDesktopFrame is a DesktopFrame that may have multiple instances all
// sharing the same buffer.
class SharedDesktopFrame : public DesktopFrame {
 public:
  virtual ~SharedDesktopFrame();

  static SharedDesktopFrame* Wrap(DesktopFrame* desktop_frame);

  // Returns the underlying instance of DesktopFrame.
  DesktopFrame* GetUnderlyingFrame();

  // Creates a clone of this object.
  SharedDesktopFrame* Share();

  // Checks if the frame is currently shared. If it returns false it's
  // guaranteed that there are no clones of the object.
  bool IsShared();

 private:
  class Core;

  SharedDesktopFrame(scoped_refptr<Core> core);

  scoped_refptr<Core> core_;

  DISALLOW_COPY_AND_ASSIGN(SharedDesktopFrame);
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_DESKTOP_CAPTURE_SHARED_DESKTOP_FRAME_H_
