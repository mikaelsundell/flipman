// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/render/render.h>
#include <flipmansdk/render/imagelayer.h>
#include <flipmansdk/render/renderengine.h>
#include <QRhiWidget>
#include <QScopedPointer>

namespace flipman::sdk::widgets {

class ViewerPrivate;

/**
 * @class Viewer
 * @brief QRhi-based widget for rendering ImageLayer stacks.
 *
 * Provides interactive zoom and pan on top of a GPU-backed render pipeline.
 */
class FLIPMANSDK_EXPORT Viewer : public QRhiWidget {
    Q_OBJECT

public:
    /**
     * @enum ZoomMode
     * @brief Defines zoom behavior.
     */
    enum ZoomMode {
        Manual,    ///< Uses interactive zoom and pan.
        FitToView  ///< Automatically fits content to viewport.
    };

public:
    /**
     * @brief Constructs a Viewer.
     *
     * @param parent Optional parent widget.
     */
    explicit Viewer(QWidget* parent = nullptr);

    /**
     * @brief Destroys the Viewer.
     */
    ~Viewer() override;

    /** @name Render */
    ///@{

    /**
     * @brief Returns the render engine used by the viewer.
     *
     * The viewer does not take ownership of the render engine.
     */
    render::RenderEngine* renderEngine() const;

    /**
     * @brief Sets the render engine used by the viewer.
     *
     * The viewer uses this engine to render into the QRhiWidget frame context.
     * Ownership remains with the caller.
     */
    void setRenderEngine(render::RenderEngine* renderEngine);

    ///@}

    /** @name View State */
    ///@{

    /**
     * @brief Returns the current zoom factor.
     */
    float zoom() const;

    /**
     * @brief Sets the zoom factor.
     *
     * Emits zoomChanged() if modified.
     */
    void setZoom(float zoom);

    /**
     * @brief Returns the current zoom mode.
     */
    ZoomMode zoomMode() const;

    /**
     * @brief Sets the zoom mode.
     */
    void setZoomMode(ZoomMode zoomMode);

    /**
     * @brief Adjusts view to fit content.
     */
    void fitToView();

    /**
     * @brief Resets zoom and pan to default state.
     */
    void resetView();

    ///@}

    /** @name Display Transform */
    ///@{

    /**
     * @brief Returns the display/surface tag used for viewer output.
     *
     * This describes the final framebuffer to the window system. It does not
     * perform color conversion. The render engine must already have produced
     * pixels matching this transform.
     */
    render::DisplayTransform displayTransform() const;

    /**
     * @brief Sets the display/surface tag used for viewer output.
     *
     * On platforms that support explicit surface color tagging, such as macOS
     * with CAMetalLayer, this value is used to describe the presented
     * framebuffer to the window system.
     */
    void setDisplayTransform(const render::DisplayTransform& transform);

    ///@}

protected:
    /** @name RHI Lifecycle */
    ///@{

    /**
     * @brief Initializes GPU resources.
     */
    void initialize(QRhiCommandBuffer* cb) override;

    /**
     * @brief Records draw commands.
     */
    void render(QRhiCommandBuffer* cb) override;

    ///@}

    /** @name Interaction */
    ///@{

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* e) override;

    ///@}

Q_SIGNALS:

    /**
     * @brief Emitted when zoom changes.
     */
    void zoomChanged(float zoom);

private:
    Q_DISABLE_COPY_MOVE(Viewer)
    friend class ViewerPrivate;
    QScopedPointer<ViewerPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::widgets

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::widgets::Viewer*)
