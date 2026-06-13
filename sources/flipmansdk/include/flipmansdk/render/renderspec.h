// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/rendersurface.h>
#include <QMatrix4x4>
#include <QMetaType>
#include <QSize>
#include <QString>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

/**
 * @class RenderSpec
 * @brief Describes one render output request.
 *
 * RenderSpec is a lightweight description of where and how a rendered frame
 * should be written. It does not own GPU resources. The render engine uses it
 * to write the computed render result to widget targets, offscreen targets,
 * readback targets, or external output paths such as DeckLink.
 */
class FLIPMANSDK_EXPORT RenderSpec {
public:
    /**
     * @enum Format
     * @brief Requested pixel format for the output.
     */
    enum class Format {
        RGBA16F,  ///< Half-float RGBA output.
        RGBA8,    ///< 8-bit RGBA output.
        UYVY8,    ///< 8-bit 4:2:2 UYVY / 2vuy output.
        V210      ///< 10-bit 4:2:2 v210 output.
    };

public:
    RenderSpec() = default;

    /**
     * @brief Returns true if the output request can be rendered.
     */
    bool isValid() const
    {
        if (!enabled || !size.isValid())
            return false;

        if (readback)
            return true;

        return surface.isValid();
    }

public:
    RenderSurface surface;            ///< Output surface used when rendering directly to an external QRhi target.
    QMatrix4x4 view;                  ///< Output transform, for example viewer pan/zoom.
    QSize size;                       ///< Output size in pixels.
    QString lut;                      ///< Optional output/calibration LUT path.
    Format format = Format::RGBA16F;  ///< Requested output pixel format.
    bool enabled = true;              ///< Whether this output should be rendered.
    bool readback = false;            ///< Whether this output should produce CPU-readable data.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderSpec)
