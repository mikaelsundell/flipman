// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QString>

namespace flipman::sdk::test {
void
init(const QString& dataDirectory, const QString& outputDirectory);
bool
testClip();
bool
testFile();
bool
testFps();
bool
testImage();
bool
testTime();
bool
testTimeRange();
bool
testMedia();
bool
testRender();
bool
testShader();
bool
testSmpte();
bool
testTimer();
bool
testPlugin();
bool
testPluginRegistry();
bool
testTimeLine();
bool
testImageClassification();
bool
testImageNV12Layout();
bool
testImagePackedLayout();
bool
testImageUint16();
bool
testImageDouble();
bool
testImagePlanar();
bool
testImageInterleaved();
bool
testImageNV12();
bool
testImageAverage();
bool
testImageThreaded();
bool
testRenderOffscreen();
bool
testRenderRoundtrip();
bool
testPluginFx();
bool
testPluginContainer();
bool
testPluginImage();
bool
testMediaState();
bool
testShaderContract();
}  // namespace flipman::sdk::test
