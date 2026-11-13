// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/flipmansdk.h>
#include <CoreVideo/CoreVideo.h>
#include <QScopedPointer>

namespace flipman::sdk::render {

class MetalImageConverterPrivate;

// Internal synchronous encoder bridge. Create/use/destroy on one thread.
// The returned IOSurface-backed BGRA buffer is +1 retained by the caller.
class FLIPMANSDK_EXPORT MetalImageConverter {
public:
    MetalImageConverter();
    ~MetalImageConverter();
    CVPixelBufferRef convert(const core::ImageBuffer& image);
    core::Error error() const;

private:
    Q_DISABLE_COPY_MOVE(MetalImageConverter)
    QScopedPointer<MetalImageConverterPrivate> p;
};

}  // namespace flipman::sdk::render
