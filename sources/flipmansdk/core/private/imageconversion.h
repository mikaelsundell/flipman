// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/core/imagebuffer.h>

namespace flipman::sdk::core::detail {

// Shared CPU kernels. No rendering device, GPU submission or readback is involved.
// Preserve the legacy identity semantics: format conversion detaches, while a
// channel-only identity conversion shares storage.
ImageBuffer
convertImage(const ImageBuffer& source, ImageFormat::Type type, int channels);
ImageBuffer
convertChannels(const ImageBuffer& source, int channels);

}  // namespace flipman::sdk::core::detail
