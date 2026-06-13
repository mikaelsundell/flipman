// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <QMetaType>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

/**
 * @class RenderContext
 * @brief Describes the QRhi device state required for rendering.
 *
 * RenderContext is a lightweight frame context. It references the active QRhi
 * used for resource creation and command recording. It does not own the QRhi.
 */
class FLIPMANSDK_EXPORT RenderContext {
public:
    /**
     * @brief Constructs an invalid render context.
     */
    RenderContext() = default;

    /**
     * @brief Returns true if the context can be used for rendering.
     */
    bool isValid() const { return rhi; }

public:
    QRhi* rhi = nullptr;  ///< Active QRhi used for this render frame.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderContext)
