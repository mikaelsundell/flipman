// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/renderlayer.h>
#include <flipmansdk/flipmansdk.h>

#include <QColor>
#include <QFile>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QSize>

#include <rhi/qrhi.h>

namespace flipman::sdk::av {

class RenderEnginePrivate;

/**
 * @class RenderEngine
 * @brief The core hardware-accelerated rendering backend for Flipman.
 *
 * RenderEngine manages the RHI (Rendering Hardware Interface) resources and
 * command recording. It translates RenderLayers into low-level draw calls
 * for backends such as Metal, Vulkan, and Direct3D.
 *
 * @note This class is typically driven by a RenderWidget or a MediaProcessor
 * during the render pass of the Qt graphics pipeline.
 */
class FLIPMANSDK_EXPORT RenderEngine : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a RenderEngine.
     * @param parent The ownership parent for the QObject tree.
     */
    explicit RenderEngine(QObject* parent = nullptr);

    /**
     * @brief Destroys the RenderEngine and releases GPU resources.
     * @note Required for the PIMPL pattern to safely delete RenderEnginePrivate.
     */
    ~RenderEngine() override;

    /** @name Rendering Lifecycle */
    ///@{
    /**
     * @brief Initializes graphics pipelines and buffers.
     * @param cb The command buffer used for resource upload and pipeline initialization.
     */
    void init(QRhiCommandBuffer* cb);

    /**
     * @brief Records draw commands for the current frame.
     * @param cb The command buffer to record into.
     */
    void render(QRhiCommandBuffer* cb);
    ///@}



    /** @name Configuration */
    ///@{
    /**
     * @brief Returns the current internal rendering resolution.
     */
    QSize resolution() const;

    /**
     * @brief Sets the target rendering resolution.
     */
    void setResolution(const QSize& resolution);

    /**
     * @brief Sets the clear color for the viewport.
     */
    void setBackground(const QColor& background);

    /**
     * @brief Sets the stack of layers to be processed and drawn.
     */
    void setRenderLayers(const QList<RenderLayer>& renderlayers);
    ///@}

    /** @name Utilities and Status */
    ///@{
    /**
     * @brief Helper utility to compile a shader file into a QShader package.
     * @param file The source shader file (e.g., .vert or .frag).
     * @return A compiled QShader object ready for RHI pipelines.
     */
    static QShader compile(const QFile& file);

    /**
     * @brief Validates if the engine is ready to record commands.
     */
    bool isValid() const;

    /**
     * @brief Resets the engine state and clears all render layers.
     */
    void reset();
    ///@}

private:
    Q_DISABLE_COPY_MOVE(RenderEngine)
    friend class RenderEnginePrivate;
    QScopedPointer<RenderEnginePrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::RenderEngine*)
