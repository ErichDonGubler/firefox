/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc.videoengineapp;

public interface IViEAndroidCallback {
    public int updateStats(int frameRateI, int bitRateI,
        int packetLoss, int frameRateO,
        int bitRateO);

    public int newIncomingResolution(int width, int height);
}
