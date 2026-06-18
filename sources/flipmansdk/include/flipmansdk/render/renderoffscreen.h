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

    /** @name Render */
    ///@{

    /**
     * @brief Returns the render engine used by the offscreen renderer.
     *
     * The offscreen renderer does not take ownership of the render engine.
     */
    render::RenderEngine* renderEngine() const;

    /**
     * @brief Sets the render engine used by the offscreen renderer.
     *
     * The offscreen renderer uses this engine to render into an offscreen render
     * target. Ownership remains with the caller.
     */
    void setRenderEngine(render::RenderEngine* renderEngine);

    ///@}

    /**
     * @brief Executes an offscreen render pass and returns a native RGB image.
     *
     * Source layers may use encoded or planar video formats, but the returned image
     * is always an interleaved RGB-like ImageBuffer that does not require decode.
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
