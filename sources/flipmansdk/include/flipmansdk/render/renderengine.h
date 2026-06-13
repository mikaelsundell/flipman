// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/imagelayer.h>
#include <flipmansdk/render/rendercontext.h>
#include <flipmansdk/render/renderoutput.h>
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
       * @brief Initializes GPU resources for the given context and render specification.
       *
       * Safe to call multiple times. The context provides the active QRhi device,
       * while the render specification describes the primary output surface,
       * size, format, and output requirements used to build compatible GPU
       * resources.
       *
       * @param context    Rendering context.
       * @param spec        Primary render specification for this render call.
       */
    bool initialize(const RenderContext& context, const RenderSpec& spec);

    /**
     * @brief Records draw commands for a render request.
     *
     * The engine renders the configured image layers using the supplied render
     * specification. The rendered result is written to the destination
     * described by @p renderSpec and to any additional render outputs
     * configured with setRenderOutputs().
     *
     * @param context       Rendering context.
     * @param spec              Primary render specification for this render call.
     * @param commandBuffer Command buffer used for recording GPU commands.
     */
    void render(const RenderContext& context, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer);

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
     * @brief Returns the image layers.
     */
    QList<ImageLayer> imageLayers() const;

    /**
     * @brief Sets the image layers to render.
     */
    void setImageLayers(const QList<ImageLayer>& imageLayers);

    ///@}

    /** @name Render Outputs */
    ///@{

    /**
     * @brief Returns the render outputs rendered after the primary pass.
     */
    QList<RenderOutput*> renderOutputs() const;

    /**
     * @brief Sets render outputs rendered after the primary pass.
     *
     * Outputs allow the same rendered result to be written to
     * multiple destinations, such as DeckLink devices, readback targets,
     * or file/export outputs. The primary output is supplied when calling
     * render().
     */
    void setRenderOutputs(const QList<RenderOutput*>& renderOutputs);

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
