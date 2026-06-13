// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/rendersurface.h>
#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

class RenderContextPrivate;

/**
 * @class RenderContext
 * @brief A lightweight, render-ready GPU execution context.
 *
 * RenderContext serves as the bridge between the active QRhi device/frame and
 * the low-level RenderEngine. It describes the GPU device used for rendering
 * and the primary surface associated with the current render frame.
 *
 * @note Because it uses QExplicitlySharedDataPointer, it is designed for cheap
 * value copies and safe handoffs between render device code and rendering code.
 */
class FLIPMANSDK_EXPORT RenderContext {
public:
    /**
     * @brief Constructs an empty RenderContext.
     */
    RenderContext();

    /**
     * @brief Copy constructor. Performs a shallow copy of the context data.
     */
    RenderContext(const RenderContext& other);

    /**
     * @brief Destroys the RenderContext.
     * @note Required for the PIMPL pattern to safely delete RenderContextPrivate.
     */
    ~RenderContext();

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the active QRhi device.
     */
    QRhi* rhi() const;

    /**
     * @brief Sets the active QRhi device.
     */
    void setRhi(QRhi* rhi);

    /**
     * @brief Returns the primary render surface for the current frame.
     */
    RenderSurface surface() const;

    /**
     * @brief Sets the primary render surface for the current frame.
     */
    void setSurface(const RenderSurface& surface);

    ///@}

    /** @name Status */
    ///@{

    /**
     * @brief Returns true if the context can be used for rendering.
     */
    bool isValid() const;

    /**
     * @brief Resets the context to an empty, invalid state.
     */
    void reset();

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy of the shared data.
     */
    RenderContext& operator=(const RenderContext& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const RenderContext& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const RenderContext& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<RenderContextPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderContext)
