/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/video_engine/test/common/video_renderer.h"

// TODO(pbos): Windows renderer
// TODO(pbos): Android renderer

#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class NullRenderer : public VideoRenderer {
  virtual void RenderFrame(const I420VideoFrame& video_frame,
                           int time_to_render_ms) OVERRIDE {}
};

VideoRenderer* VideoRenderer::Create(const char* window_title, size_t width,
                                     size_t height) {
  VideoRenderer* renderer = CreatePlatformRenderer(window_title, width, height);
  if (renderer != NULL) {
    // TODO(mflodman) Add a warning log.
    return renderer;
  }
  return new NullRenderer();
}
}  // test
}  // webrtc
