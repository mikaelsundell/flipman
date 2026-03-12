// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/render/imagelayer.h>

#include <QColor>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QSize>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

class RenderEnginePrivate;

/**
 * @class RenderEngine
 * @brief GPU renderer responsible for executing draw calls.
 *
 * Manages QRhi resources and translates ImageLayers into GPU commands.
 * Can render to widget or offscreen targets.
 */
class FLIPMANSDK_EXPORT RenderEngine : public QObject {
    Q_OBJECT
public:
    /**
     * @struct Context
     * @brief Rendering state required for a frame.
     *
     * The engine does not take ownership of the referenced objects.
     */
    struct Context {
        Context()
            : rhi(nullptr)
            , renderTarget(nullptr)
            , renderPassDescriptor(nullptr)
        {}

        QRhi* rhi;
        QRhiRenderTarget* renderTarget;
        QRhiRenderPassDescriptor* renderPassDescriptor;
        QMatrix4x4 view;
        QSize size;

        /**
         * @brief Returns true if the context is valid.
         */
        bool isValid() const { return rhi && renderTarget && renderPassDescriptor && size.isValid(); }
    };

public:
    /**
     * @brief Constructs a RenderEngine.
     */
    explicit RenderEngine(QObject* parent = nullptr);

    /**
     * @brief Destroys the RenderEngine.
     */
    ~RenderEngine() override;

    /**
     * @brief Returns whether the render engine has been initialized.
     *
     * @return @c true if GPU resources are ready for rendering, otherwise @c false.
     */
    bool initialized() const;

    /**
     * @brief Initializes GPU resources for the given context.
     *
     * Safe to call multiple times.
     */
    bool initialize(const Context& context);

    /**
     * @brief Records draw commands for active layers.
     *
     * @param context Rendering context.
     * @param cb      Command buffer.
     */
    void render(const Context& context, QRhiCommandBuffer* cb);

    /** @name Configuration */
    ///@{

    /**
     * @brief Returns the current rendering resolution.
     */
    QSize resolution() const;

    /**
     * @brief Sets the target rendering resolution.
     */
    void setResolution(const QSize& resolution);

    /**
     * @brief Returns the current background color.
     */
    QColor background() const;

    /**
     * @brief Sets the background clear color.
     */
    void setBackground(const QColor& background);

    /**
     * @brief Sets the image layers to render.
     */
    void setImageLayers(const QList<ImageLayer>& imageLayers);

    ///@}

    /**
     * @brief Returns generation error, if any.
     */
    core::Error error() const;

    /**
     * @brief Returns true if GPU resources are ready.
     */
    bool isValid() const;

    /**
     * @brief Releases GPU resources and clears state.
     */
    void reset();

private:
    Q_DISABLE_COPY_MOVE(RenderEngine)
    QScopedPointer<RenderEnginePrivate> p;
};

}  // namespace flipman::sdk::render

Q_DECLARE_METATYPE(flipman::sdk::render::RenderEngine*)
