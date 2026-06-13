// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <QMetaType>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

/**
 * @class RenderSurface
 * @brief Describes a QRhi render surface.
 *
 * RenderSurface references an external QRhi render target and its compatible
 * render pass descriptor. It does not own the referenced objects.
 */
class FLIPMANSDK_EXPORT RenderSurface {
public:
    /**
     * @brief Constructs an invalid render surface.
     */
    RenderSurface() = default;

    /**
     * @brief Returns true if the surface can be rendered into.
     */
    bool isValid() const { return renderTarget; }

public:
    QRhiRenderTarget* renderTarget = nullptr;  ///< External render target.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderSurface)
