// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/renderlayer.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/flipmansdk.h>

#include <QRhiWidget>
#include <QScopedPointer>

namespace flipman::sdk::widgets {

class RenderWidgetPrivate;

/**
 * @class RenderWidget
 * @brief A high-performance graphics widget for rendering image sequences and AV layers.
 *
 * RenderWidget leverages QRhiWidget to provide hardware-accelerated rendering
 * across different graphics APIs (Metal, Vulkan, Direct3D, OpenGL). It manages
 * a stack of RenderLayers and handles user interaction for zooming and panning
 * within the render viewport.
 */
class FLIPMANSDK_EXPORT RenderWidget : public QRhiWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs a RenderWidget.
     * @param parent The parent widget.
     */
    RenderWidget(QWidget* parent = nullptr);

    /**
     * @brief Destroys the RenderWidget and releases graphics resources.
     * @note Required for the PIMPL pattern to safely delete RenderWidgetPrivate.
     */
    ~RenderWidget() override;

    /** @name Configuration */
    ///@{
    /**
     * @brief Returns the logical resolution of the internal render buffer.
     * This may differ from the widget's physical size due to scaling/letterboxing.
     */
    QSize resolution() const;

    /**
     * @brief Sets the target resolution for the rendering pipeline.
     * @param resolution The width and height in pixels.
     */
    void setResolution(const QSize& resolution);

    /**
     * @brief Sets the clear color for the render background.
     * @param background The color used for empty areas of the viewport.
     */
    void setBackground(const QColor& background);

    /**
     * @brief Updates the stack of layers to be drawn.
     * @param renderlayers A list of layers containing image data or graphics primitives.
     */
    void setRenderLayers(const QList<av::RenderLayer>& renderLayers);
    ///@}



protected:
    /** @name RHI Lifecycle */
    ///@{
    /**
     * @brief Sets up graphics resources (pipelines, buffers) for the RHI backend.
     * Called automatically by the Qt rendering thread before the first frame.
     */
    void initialize(QRhiCommandBuffer* cb) override;

    /**
     * @brief Records drawing commands into the command buffer.
     * Performed on the rendering thread to produce the final frame.
     */
    void render(QRhiCommandBuffer* cb) override;
    ///@}

    /** @name Interaction Overrides */
    ///@{
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    ///@}

private:
    Q_DISABLE_COPY_MOVE(RenderWidget)
    friend class RenderWidgetPrivate;
    QScopedPointer<RenderWidgetPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::widgets

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::widgets::RenderWidget*)
