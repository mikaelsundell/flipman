// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/render/imagelayer.h>

#include <QColor>
#include <QImage>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QSize>

namespace flipman::sdk::render {

class RenderOffscreenPrivate;

/**
 * @class RenderOffscreen
 * @brief High-level offscreen renderer for producing images.
 *
 * Owns the rendering backend and executes offscreen render passes.
 * Intended for frame export, thumbnails, and batch rendering.
 */
class FLIPMANSDK_EXPORT RenderOffscreen : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs an uninitialized RenderOffscreen.
     *
     * initialize() must be called before render().
     */
    explicit RenderOffscreen(QObject* parent = nullptr);

    /**
     * @brief Destroys the RenderOffscreen and releases GPU resources.
     */
    ~RenderOffscreen() override;

    /**
     * @brief Initializes the rendering backend.
     *
     * @param size Target render resolution.
     * @return true if initialization succeeded.
     */
    bool initialize(const QSize& size);

    /**
     * @brief Sets the background clear color.
     */
    void setBackground(const QColor& color);

    /**
     * @brief Sets the image layers to render.
     *
     * Layers are processed in order.
     */
    void setImageLayers(const QList<ImageLayer>& imageLayers);

    /**
     * @brief Executes a render pass and returns the resulting image.
     *
     * @return Rendered image.
     */
    core::ImageBuffer render();

private:
    Q_DISABLE_COPY_MOVE(RenderOffscreen)
    QScopedPointer<RenderOffscreenPrivate> p;
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderOffscreen*)
