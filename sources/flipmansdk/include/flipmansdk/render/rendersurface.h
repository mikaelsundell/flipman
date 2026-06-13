// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

class RenderSurfacePrivate;

/**
 * @class RenderSurface
 * @brief A lightweight, non-owning reference to a QRhi render target.
 *
 * RenderSurface describes an external render target that can be used as a
 * destination for a render pass. It does not own the referenced QRhi objects
 * and does not manage their lifetime.
 *
 * @note Because it uses QExplicitlySharedDataPointer, it is designed for cheap
 * value copies and safe handoffs between UI/output configuration and rendering
 * code.
 */
class FLIPMANSDK_EXPORT RenderSurface {
public:
    /**
     * @brief Constructs an invalid RenderSurface.
     */
    RenderSurface();

    /**
     * @brief Copy constructor. Performs a shallow copy of the surface data.
     */
    RenderSurface(const RenderSurface& other);

    /**
     * @brief Destroys the RenderSurface.
     * @note Required for the PIMPL pattern to safely delete RenderSurfacePrivate.
     */
    ~RenderSurface();

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the external non-owned render target.
     */
    QRhiRenderTarget* renderTarget() const;

    /**
     * @brief Sets the external non-owned render target.
     */
    void setRenderTarget(QRhiRenderTarget* renderTarget);

    ///@}

    /** @name Status */
    ///@{

    /**
     * @brief Returns true if the surface references a render target.
     */
    bool isValid() const;

    /**
     * @brief Resets the surface to an empty, invalid state.
     */
    void reset();

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy of the shared data.
     */
    RenderSurface& operator=(const RenderSurface& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const RenderSurface& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const RenderSurface& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<RenderSurfacePrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderSurface)
