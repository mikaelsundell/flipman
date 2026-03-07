// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/render/imageeffect.h>

#include <QExplicitlySharedDataPointer>
#include <QMatrix4x4>
#include <QMetaType>

namespace flipman::sdk::render {

class ImageLayerPrivate;

/**
 * @class ImageLayer
 * @brief A lightweight, render-ready data packet for the graphics engine.
 *
 * ImageLayer serves as the bridge between the high-level Clip logic and the
 * low-level RenderEngine. It encapsulates a decoded image buffer, its
 * associated visual effects, and its final transformation matrix.
 *
 * @note Because it uses QExplicitlySharedDataPointer, it is designed for
 * thread-safe handoffs from the decoding/UI thread to the rendering thread.
 */
class FLIPMANSDK_EXPORT ImageLayer {
public:
    /**
     * @brief Constructs an empty ImageLayer.
     */
    ImageLayer();

    /**
     * @brief Copy constructor. Performs a shallow copy of the layer data.
     */
    ImageLayer(const ImageLayer& other);

    /**
     * @brief Destroys the ImageLayer.
     * @note Required for the PIMPL pattern to safely delete ImageLayerPrivate.
     */
    ~ImageLayer();

    /** @name Attributes */
    ///@{
    /**
     * @brief Returns the decoded pixel data associated with this layer.
     */
    core::ImageBuffer image() const;

    /**
     * @brief Sets the pixel data for this layer.
     */
    void setImage(const core::ImageBuffer& image);

    /**
     * @brief Returns the effect configuration to be applied during the render pass.
     */
    ImageEffect imageEffect() const;

    /**
     * @brief Sets the visual effect configuration.
     */
    void setImageEffect(const ImageEffect& imageEffect);

    /**
     * @brief Returns the 4x4 transformation matrix (Model Matrix) for this layer.
     */
    QMatrix4x4 transform() const;

    /**
     * @brief Sets the 4x4 transformation matrix.
     */
    void setTransform(const QMatrix4x4& transform);
    ///@}



    /** @name Status */
    ///@{
    /**
     * @brief Returns any error associated with the layer data or buffers.
     */
    core::Error error() const;

    /**
     * @brief Resets the layer to an empty, uninitialized state.
     */
    void reset();
    ///@}

    /** @name Operators */
    ///@{
    /**
     * @brief Assignment operator. Performs a shallow copy of the shared data.
     */
    ImageLayer& operator=(const ImageLayer& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const ImageLayer& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const ImageLayer& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<ImageLayerPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::ImageLayer)
