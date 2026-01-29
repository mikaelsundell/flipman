// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/rendereffect.h>
#include <flipmansdk/core/imagebuffer.h>

#include <QExplicitlySharedDataPointer>
#include <QMatrix4x4>

namespace flipman::sdk::av {

class RenderLayerPrivate;

/**
 * @class RenderLayer
 * @brief A lightweight, render-ready data packet for the graphics engine.
 * * RenderLayer serves as the bridge between the high-level Clip logic and the
 * low-level RenderEngine. It encapsulates a decoded image buffer, its
 * associated visual effects, and its final transformation matrix.
 * * @note Because it uses QExplicitlySharedDataPointer, it is designed for
 * thread-safe handoffs from the decoding/UI thread to the rendering thread.
 */
class FLIPMANSDK_EXPORT RenderLayer {
public:
    /**
     * @brief Constructs an empty RenderLayer.
     */
    RenderLayer();

    /**
     * @brief Copy constructor. Performs a shallow copy of the layer data.
     */
    RenderLayer(const RenderLayer& other);

    /**
     * @brief Destroys the RenderLayer.
     */
    virtual ~RenderLayer();

    /** @name Data Access */
    ///@{
    /**
     * @brief Returns the decoded pixel data associated with this layer.
     */
    core::ImageBuffer image() const;

    /**
     * @brief Returns the effect configuration to be applied during the render pass.
     */
    RenderEffect renderEffect() const;

    /**
     * @brief Returns the 4x4 transformation matrix (Model Matrix) for this layer.
     */
    QMatrix4x4 transform() const;
    ///@}

    /** @name Status */
    ///@{
    core::Error error() const;
    void reset();
    ///@}

    /** @name Setters */
    ///@{
    void setImage(const core::ImageBuffer& image);
    void setRenderEffect(const RenderEffect& renderEffect);
    void setTransform(const QMatrix4x4& transform);
    ///@}

    /** @name Operators */
    ///@{
    RenderLayer& operator=(const RenderLayer& other);
    bool operator==(const RenderLayer& other) const;
    bool operator!=(const RenderLayer& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<RenderLayerPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::RenderLayer)
