// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/render/imagelayer.h>

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

    /** @name Configuration */
    ///@{

    /**
     * @brief Returns the internal render resolution.
     */
    QSize resolution() const;

    /**
     * @brief Sets the internal render resolution.
     */
    void setResolution(const QSize& resolution);

    /**
     * @brief Sets the background clear color.
     */
    void setBackground(const QColor& background);

    /**
     * @brief Sets the layers to render.
     */
    void setImageLayers(const QList<render::ImageLayer>& imageLayers);

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
