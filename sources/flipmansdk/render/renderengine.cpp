// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderengine.h>

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>
#include <flipmansdk/render/shadercompiler.h>
#include <flipmansdk/render/shadercontract.h>
#include <flipmansdk/render/shaderparser.h>

#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QMatrix4x4>

#include <QCryptographicHash>
#include <QHash>

//#define RENDERENGINE_STATS 0
//#define RENDERENGINE_TRACE 0

#if defined(RENDERENGINE_STATS)
#    define RE_STATS_ENABLED 1
#else
#    define RE_STATS_ENABLED 0
#endif

#if defined(RENDERENGINE_TRACE)
#    define RE_TRACE_ENABLED 1
#else
#    define RE_TRACE_ENABLED 0
#endif

#if RE_TRACE_ENABLED
#    define RE_TRACE qDebug
#else
struct RenderEngineNoDebugStream {
    template<typename T> RenderEngineNoDebugStream& operator<<(const T&) { return *this; }
};
static inline RenderEngineNoDebugStream
reNoDebugStream()
{
    return {};
}
#    define RE_TRACE reNoDebugStream
#endif

namespace flipman::sdk::render {

class RenderEnginePrivate : public QSharedData {
public:
    RenderEnginePrivate();
    bool initResources(const RenderEngine::Context& context);
    void updateRenderStates(const RenderEngine::Context&, QRhiResourceUpdateBatch* updates);
    void updateBlit(const RenderEngine::Context&, QRhiResourceUpdateBatch*);
    bool updateBlitResources(const RenderEngine::Context& context);
    void freeResources();
    void render(const RenderEngine::Context& context, QRhiCommandBuffer* commandBuffer);
    void renderScene(const RenderEngine::Context&, QRhiCommandBuffer*);
    void renderBlit(const RenderEngine::Context&, QRhiCommandBuffer*);
    QString loadShader(const QString& name);
    QShader compileShader(const QString& source, QShader::Stage stage);
    QRectF aspectFit(const QSize& src, const QSize& dst);

public:
    struct RenderState {
        enum class TextureType { Unknown, UInt8, Half, Float, Nv12 };
        TextureType textureType = TextureType::Unknown;
        std::unique_ptr<QRhiTexture> texture0;
        std::unique_ptr<QRhiTexture> texture1;
        std::unique_ptr<QRhiShaderResourceBindings> shaderBindings;
        std::unique_ptr<QRhiBuffer> globalBuffer;
        std::unique_ptr<QRhiBuffer> effectBuffer;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        core::ImageBuffer imageData;
        core::ImageBuffer imageData0;
        core::ImageBuffer imageData1;
        QByteArray effectParameterData;
        void reset()
        {
            textureType = TextureType::Unknown;
            texture0.reset();
            texture1.reset();
            shaderBindings.reset();
            globalBuffer.reset();
            effectBuffer.reset();
            pipeline.reset();
            imageData.reset();
            imageData0.reset();
            imageData1.reset();
            effectParameterData.clear();
        }
        bool initTextures(const core::ImageBuffer& image, QRhi* rhi)
        {
            if (!rhi || !image.isValid())
                return false;
            if (textureType == TextureType::Nv12) {
                const QSize ySize = image.planeSize(0);
                const QSize uvSize = image.planeSize(1);
                if (!texture0 || texture0->pixelSize() != ySize || texture0->format() != QRhiTexture::R8) {
                    texture0.reset(rhi->newTexture(QRhiTexture::R8, ySize));
                    if (!texture0->create())
                        return false;
                }
                if (!texture1 || texture1->pixelSize() != uvSize || texture1->format() != QRhiTexture::RG8) {
                    texture1.reset(rhi->newTexture(QRhiTexture::RG8, uvSize));
                    if (!texture1->create())
                        return false;
                }
                return true;
            }
            const core::ImageBuffer& uploadImage = imageData0.isValid() ? imageData0 : image;
            const QSize size(uploadImage.dataWindow().width(), uploadImage.dataWindow().height());
            const QRhiTexture::Format format = toTextureFormat(textureType);

            if (!texture0 || texture0->pixelSize() != size || texture0->format() != format) {
                texture0.reset(rhi->newTexture(format, size));
                if (!texture0->create())
                    return false;
            }
            texture1.reset();
            return true;
        }

        bool prepareUpload(const core::ImageBuffer& image)
        {
            imageData0.reset();
            imageData1.reset();
            if (!image.isValid())
                return false;

            if (textureType == TextureType::Nv12) {
                imageData0 = image;
                imageData1 = image;
                return true;
            }
            if (image.channels() == 4) {
                imageData0 = image;
            }
            else {
                imageData0 = core::ImageBuffer::convert(image, image.imageFormat().type(), 4);
            }
            return imageData0.isValid();
        }
        quint64 uploadTextures(QRhiResourceUpdateBatch* updates)
        {
            if (!updates || !texture0)
                return 0;

            if (textureType == TextureType::Nv12) {
                if (!texture1 || !imageData0.isValid() || !imageData1.isValid())
                    return 0;

                QRhiTextureSubresourceUploadDescription yUpload(imageData0.planeData(0),
                                                                static_cast<quint32>(imageData0.planeByteSize(0)));
                yUpload.setDataStride(static_cast<quint32>(imageData0.planeStride(0)));
                yUpload.setSourceSize(imageData0.planeSize(0));

                updates->uploadTexture(texture0.get(),
                                       QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, yUpload) }));

                QRhiTextureSubresourceUploadDescription uvUpload(imageData1.planeData(1),
                                                                 static_cast<quint32>(imageData1.planeByteSize(1)));
                uvUpload.setDataStride(static_cast<quint32>(imageData1.planeStride(1)));
                uvUpload.setSourceSize(imageData1.planeSize(1));

                updates->uploadTexture(texture1.get(),
                                       QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, uvUpload) }));

                return quint64(imageData0.planeByteSize(0)) + quint64(imageData1.planeByteSize(1));
            }

            if (!imageData0.isValid())
                return 0;

            QRhiTextureSubresourceUploadDescription subres(imageData0.data(),
                                                           static_cast<quint32>(imageData0.byteSize()));
            subres.setDataStride(static_cast<quint32>(imageData0.strideSize()));
            subres.setSourceSize(QSize(imageData0.dataWindow().width(), imageData0.dataWindow().height()));

            updates->uploadTexture(texture0.get(),
                                   QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, subres) }));

            return quint64(imageData0.byteSize());
        }
        static QRhiTexture::Format toTextureFormat(TextureType type)
        {
            switch (type) {
            case TextureType::UInt8: return QRhiTexture::RGBA8;
            case TextureType::Half: return QRhiTexture::RGBA16F;
            case TextureType::Float: return QRhiTexture::RGBA32F;
            case TextureType::Unknown:
            case TextureType::Nv12:
            default: return QRhiTexture::RGBA16F;
            }
        }
        static TextureType toTextureType(const core::ImageBuffer& image)
        {
            if (!image.isValid())
                return TextureType::Unknown;
            if (image.packing() == core::ImageBuffer::Packing::BiPlanar && image.planeCount() == 2
                && image.subsampling() == core::ImageBuffer::Subsampling::CS420) {
                return TextureType::Nv12;
            }
            switch (image.imageFormat().type()) {
            case core::ImageFormat::Type::UInt8: return TextureType::UInt8;
            case core::ImageFormat::Type::Half: return TextureType::Half;
            case core::ImageFormat::Type::Float: return TextureType::Float;
            default: return TextureType::Unknown;
            }
        }
    };
    QByteArray shaderHash(const QString& text) const;
    QString buildLayerShaderKey(RenderState::TextureType textureType, const ShaderDefinition* effectDefinition) const;
    QString buildLayerShaderSource(RenderState::TextureType textureType, const ShaderDefinition* effectDefinition);
    struct FrameStats {
        quint64 frameIndex = 0;
        int layerCount = 0;
        int texturesInitialized = 0;
        int textureUploads = 0;
        quint64 textureUploadBytes = 0;
        int globalBufferUpdates = 0;
        quint64 globalBufferUploadBytes = 0;
        int effectBufferCreates = 0;
        int effectBufferUpdates = 0;
        quint64 effectBufferUploadBytes = 0;
        int shaderBindingsCreated = 0;
        int pipelinesCreated = 0;
        int shaderSourceCacheHits = 0;
        int shaderSourceCacheMisses = 0;
        int generatedShaderSourceCacheHits = 0;
        int generatedShaderSourceCacheMisses = 0;
        int shaderCacheHits = 0;
        int shaderCacheMisses = 0;
        qint64 updateRenderStatesNs = 0;
        qint64 updateBlitNs = 0;
        qint64 renderSceneNs = 0;
        qint64 renderBlitNs = 0;
        qint64 renderFrameNs = 0;
        void reset(quint64 frame, int layers)
        {
            *this = {};
            frameIndex = frame;
            layerCount = layers;
        }
    };
    void resetFrameStats();
    void logFrameStats() const;
    struct Data {
        QRhi* deviceRhi = nullptr;
        QRhiRenderPassDescriptor* deviceRenderPassDescriptor = nullptr;
        std::unique_ptr<QRhiTexture> renderTexture;
        std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
        std::unique_ptr<QRhiRenderPassDescriptor> renderPassDescriptor;
        std::unique_ptr<QRhiGraphicsPipeline> blitPipeline;
        std::unique_ptr<QRhiShaderResourceBindings> blitShaderBindings;
        std::vector<RenderState> renderStates;
        std::unique_ptr<QRhiBuffer> quadBuffer;
        std::unique_ptr<QRhiBuffer> mvpBuffer;
        std::unique_ptr<QRhiSampler> sampler;
        std::vector<float> quad;
        QRhiVertexInputLayout quadLayout;
        QSize size;
        QSize resolution = QSize(1920, 1080);
        QColor background = core::style()->color(core::Style::Viewer);
        QList<ImageLayer> imageLayers;
        bool quadBufferUploaded = false;
        bool mvpBufferUploaded = false;
        bool valid = false;
        quint64 frameIndex = 0;
        QHash<QString, QString> shaderSourceCache;
        QHash<QString, QString> generatedShaderSourceCache;
        QHash<QString, QShader> shaderCache;
        FrameStats stats;
        core::Error error;
    };
    Data d;
};

RenderEnginePrivate::RenderEnginePrivate() {}

bool
RenderEnginePrivate::initResources(const RenderEngine::Context& renderContext)
{
    if (!renderContext.isValid())
        return false;

    d.deviceRhi = renderContext.rhi;
    d.deviceRenderPassDescriptor = renderContext.renderPassDescriptor;
    d.size = renderContext.size;

    d.quad = {
        // pos(x,y,z)      uv
        -1.f, -1.f, 0.f, 0.f, 0.f, 1.f, -1.f, 0.f, 1.f, 0.f, -1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0.f, 1.f, 1.f,
    };

    d.quadBuffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                              static_cast<quint32>(d.quad.size()) * sizeof(float)));
    if (!d.quadBuffer || !d.quadBuffer->create()) {
        d.error = core::Error("renderengine", "could not create quad buffer");
        return false;
    }

    d.mvpBuffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(float) * 16));
    if (!d.mvpBuffer || !d.mvpBuffer->create()) {
        d.error = core::Error("renderengine", "could not create mvp buffer");
        return false;
    }

    d.sampler.reset(d.deviceRhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                            QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    if (!d.sampler || !d.sampler->create()) {
        d.error = core::Error("renderengine", "could not create sampler");
        return false;
    }

    d.quadLayout.setBindings({ { 5 * sizeof(float) } });
    d.quadLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) },
    });

    d.renderTexture.reset(d.deviceRhi->newTexture(QRhiTexture::RGBA16F, d.resolution, 1,
                                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!d.renderTexture || !d.renderTexture->create()) {
        d.error = core::Error("renderengine", "could not create render texture");
        return false;
    }

    QRhiTextureRenderTargetDescription textureRenderTargetDesc(QRhiColorAttachment(d.renderTexture.get()));
    d.renderTarget.reset(d.deviceRhi->newTextureRenderTarget(textureRenderTargetDesc));
    d.renderPassDescriptor.reset(d.renderTarget->newCompatibleRenderPassDescriptor());
    d.renderTarget->setRenderPassDescriptor(d.renderPassDescriptor.get());

    if (!d.renderTarget || !d.renderTarget->create()) {
        d.error = core::Error("renderengine", "could not create render target");
        return false;
    }

    if (!updateBlitResources(renderContext))
        return false;

    d.renderStates.clear();
    d.quadBufferUploaded = false;
    d.mvpBufferUploaded = false;
    d.valid = true;
    return true;
}

void
RenderEnginePrivate::updateRenderStates(const RenderEngine::Context& context, QRhiResourceUpdateBatch* updates)
{
    Q_UNUSED(context)

    if (!updates || !d.renderTexture || !d.deviceRhi)
        return;

    struct Global {
        QMatrix4x4 mvp;
        QVector2D resolution;
        float time;
        float pad0;
    };

    const quint64 frameIndex = d.frameIndex;

    RE_TRACE() << "renderengine: frame: " << frameIndex;
    RE_TRACE() << "renderengine: updateRenderStates frame" << frameIndex << "layers" << d.imageLayers.size();

    if (d.renderStates.size() != size_t(d.imageLayers.size())) {
        RE_TRACE() << "renderengine: resizing render states from" << d.renderStates.size() << "to"
                   << d.imageLayers.size();
        d.renderStates.clear();
        d.renderStates.resize(size_t(d.imageLayers.size()));
    }

    const QSize targetSize = d.renderTexture->pixelSize();

    for (int i = 0; i < d.imageLayers.size(); ++i) {
        RenderState& renderState = d.renderStates[size_t(i)];
        const ImageLayer& imageLayer = d.imageLayers[i];
        const core::ImageBuffer image = imageLayer.image();

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "begin";

        if (!image.isValid()) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "image invalid, resetting state";
            renderState.reset();
            continue;
        }

        const ImageEffect imageEffect = imageLayer.imageEffect();
        const bool hasEffect = imageEffect.isValid();

        ShaderDefinition effectDefinition;
        if (hasEffect)
            effectDefinition = imageEffect.shaderDefinition();

        const QSize texSize(image.dataWindow().width(), image.dataWindow().height());

        const RenderState::TextureType newType = RenderState::toTextureType(image);
        const bool typeChanged = renderState.textureType != newType;

        const bool imageLayoutChanged = typeChanged || renderState.imageData.packing() != image.packing()
                                        || renderState.imageData.subsampling() != image.subsampling()
                                        || renderState.imageData.imageFormat() != image.imageFormat()
                                        || renderState.imageData.channels() != image.channels()
                                        || renderState.imageData.dataWindow() != image.dataWindow()
                                        || renderState.imageData.displayWindow() != image.displayWindow();

        const bool imageContentChanged = !renderState.imageData.isValid() || !image.isAllocated()
                                         || !renderState.imageData.isAllocated()
                                         || renderState.imageData.data() != image.data();

        const bool imageChanged = imageLayoutChanged || imageContentChanged;

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "typeChanged" << typeChanged
                   << "imageLayoutChanged" << imageLayoutChanged << "imageContentChanged" << imageContentChanged
                   << "hasEffect" << hasEffect;

        renderState.textureType = newType;

        if (!renderState.prepareUpload(image)) {
            qWarning() << "renderengine: failed to prepare upload for image layer" << i;
            continue;
        }

        const bool needsTextures = !renderState.texture0 || imageLayoutChanged
                                   || (renderState.textureType == RenderState::TextureType::Nv12
                                       && !renderState.texture1);

        if (needsTextures) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "initializing textures";

            if (!renderState.initTextures(image, d.deviceRhi)) {
                qWarning() << "renderengine: failed to initialize textures for layer" << i;
                continue;
            }

#if RE_STATS_ENABLED
            ++d.stats.texturesInitialized;
#endif

            renderState.shaderBindings.reset();
            renderState.pipeline.reset();
        }

        if (imageChanged || needsTextures) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "uploading textures";

            const quint64 uploadedBytes = renderState.uploadTextures(updates);

#if RE_STATS_ENABLED
            ++d.stats.textureUploads;
            d.stats.textureUploadBytes += uploadedBytes;
#endif
        }

        if (!renderState.globalBuffer) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating global buffer";

            renderState.globalBuffer.reset(
                d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(Global)));

            if (!renderState.globalBuffer->create()) {
                qWarning() << "renderengine: failed to create global buffer for layer" << i;
                renderState.globalBuffer.reset();
                continue;
            }

            renderState.shaderBindings.reset();
            renderState.pipeline.reset();
        }

        const QRectF fit = aspectFit(texSize, targetSize);
        const float sx = float(fit.width()) / float(targetSize.width());
        const float sy = float(fit.height()) / float(targetSize.height());
        const float tx = (fit.x() / float(targetSize.width())) * 2.0f;
        const float ty = (fit.y() / float(targetSize.height())) * 2.0f;

        QMatrix4x4 mvp;
        mvp.setToIdentity();
        mvp.translate(-1.0f + sx + tx, -1.0f + sy + ty);
        mvp.scale(sx, sy);

        Global global;
        global.mvp = mvp;
        global.resolution = QVector2D(texSize.width(), texSize.height());
        global.time = 0.0f;
        global.pad0 = 0.0f;

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "updating global buffer";
        updates->updateDynamicBuffer(renderState.globalBuffer.get(), 0, sizeof(Global), &global);

#if RE_STATS_ENABLED
        ++d.stats.globalBufferUpdates;
        d.stats.globalBufferUploadBytes += sizeof(Global);
#endif

        if (hasEffect) {
            const auto& params = effectDefinition.descriptor().parameters;
            const qsizetype effectBufferSize = params.size() * 16;
            const bool recreateEffectBuffer = !renderState.effectBuffer
                                              || renderState.effectBuffer->size() != effectBufferSize;

            if (recreateEffectBuffer) {
                RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating effect buffer size"
                           << effectBufferSize;

                renderState.effectBuffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                                      static_cast<quint32>(effectBufferSize)));

                if (!renderState.effectBuffer->create()) {
                    qWarning() << "renderengine: failed to create parameter buffer for layer" << i;
                    renderState.effectBuffer.reset();
                    renderState.effectParameterData.clear();
                    continue;
                }

#if RE_STATS_ENABLED
                ++d.stats.effectBufferCreates;
#endif

                renderState.effectParameterData.clear();
                renderState.shaderBindings.reset();
                renderState.pipeline.reset();
            }

            QByteArray paramData(effectBufferSize, 0);
            for (int p = 0; p < params.size(); ++p) {
                const float value = params[p].defaultValue.toFloat();
                memcpy(paramData.data() + p * 16, &value, sizeof(float));
            }

            if (renderState.effectBuffer && renderState.effectParameterData != paramData) {
                RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "updating effect buffer";

                updates->updateDynamicBuffer(renderState.effectBuffer.get(), 0, static_cast<quint32>(paramData.size()),
                                             paramData.constData());

#if RE_STATS_ENABLED
                ++d.stats.effectBufferUpdates;
                d.stats.effectBufferUploadBytes += quint64(paramData.size());
#endif

                renderState.effectParameterData = paramData;
            }
            else {
                RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "effect buffer unchanged";
            }
        }
        else if (renderState.effectBuffer) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "removing effect buffer";
            renderState.effectBuffer.reset();
            renderState.effectParameterData.clear();
            renderState.shaderBindings.reset();
            renderState.pipeline.reset();
        }

        if (!renderState.shaderBindings) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating shader bindings";

            QVector<QRhiShaderResourceBinding> bindings;

            bindings << QRhiShaderResourceBinding::uniformBuffer(0,
                                                                 QRhiShaderResourceBinding::VertexStage
                                                                     | QRhiShaderResourceBinding::FragmentStage,
                                                                 renderState.globalBuffer.get());

            if (renderState.textureType == RenderState::TextureType::Nv12) {
                bindings << QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                                      renderState.texture0.get(), d.sampler.get());

                bindings << QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage,
                                                                      renderState.texture1.get(), d.sampler.get());
            }
            else {
                bindings << QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                                      renderState.texture0.get(), d.sampler.get());
            }

            if (renderState.effectBuffer) {
                bindings << QRhiShaderResourceBinding::uniformBuffer(3, QRhiShaderResourceBinding::FragmentStage,
                                                                     renderState.effectBuffer.get());
            }

            renderState.shaderBindings.reset(d.deviceRhi->newShaderResourceBindings());
            renderState.shaderBindings->setBindings(bindings.begin(), bindings.end());

            if (!renderState.shaderBindings->create()) {
                qWarning() << "renderengine: failed to create shader bindings for layer" << i;
                renderState.shaderBindings.reset();
                continue;
            }

#if RE_STATS_ENABLED
            ++d.stats.shaderBindingsCreated;
#endif
        }
        else {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "shader bindings reused";
        }

        if (!renderState.pipeline) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating pipeline";

            renderState.pipeline.reset(d.deviceRhi->newGraphicsPipeline());
            renderState.pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

            const ShaderDefinition* effectDefinitionPtr = hasEffect ? &effectDefinition : nullptr;
            const QString fragmentSource = buildLayerShaderSource(renderState.textureType, effectDefinitionPtr);
            if (fragmentSource.isEmpty()) {
                qWarning() << "renderengine: failed to build layer shader source:" << d.error.message();
                renderState.pipeline.reset();
                continue;
            }

            const QShader fragmentShader = compileShader(fragmentSource, QShader::FragmentStage);
            if (!fragmentShader.isValid()) {
                qWarning() << "renderengine: fragment shader compilation failed:" << d.error.message();
                renderState.pipeline.reset();
                continue;
            }

            const QString transformSource = loadShader("transform");
            if (transformSource.isEmpty()) {
                qWarning() << "renderengine: could not load transform shader";
                renderState.pipeline.reset();
                continue;
            }

            const QShader vertexShader = compileShader(transformSource, QShader::VertexStage);
            if (!vertexShader.isValid()) {
                qWarning() << "renderengine: vertex shader compilation failed:" << d.error.message();
                renderState.pipeline.reset();
                continue;
            }

            renderState.pipeline->setShaderStages(
                { { QRhiShaderStage::Vertex, vertexShader }, { QRhiShaderStage::Fragment, fragmentShader } });

            renderState.pipeline->setVertexInputLayout(d.quadLayout);

            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

            renderState.pipeline->setTargetBlends({ blend });
            renderState.pipeline->setShaderResourceBindings(renderState.shaderBindings.get());
            renderState.pipeline->setRenderPassDescriptor(d.renderPassDescriptor.get());

            if (!renderState.pipeline->create()) {
                qWarning() << "renderengine: failed to create pipeline for layer" << i;
                renderState.pipeline.reset();
                continue;
            }

#if RE_STATS_ENABLED
            ++d.stats.pipelinesCreated;
#endif
        }
        else {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "pipeline reused";
        }

        renderState.imageData = image;

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "done";
    }
}

void
RenderEnginePrivate::updateBlit(const RenderEngine::Context& context, QRhiResourceUpdateBatch* updates)
{
    if (!updates)
        return;

    const QSize widgetSize = context.size;
    const QSize textureSize = d.renderTexture->pixelSize();

    if (widgetSize.isEmpty() || textureSize.isEmpty())
        return;

    const float sx = float(textureSize.width()) / float(widgetSize.width());
    const float sy = float(textureSize.height()) / float(widgetSize.height());

    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.scale(sx, sy);
    QMatrix4x4 finalMatrix = context.view * matrix;

    updates->updateDynamicBuffer(d.mvpBuffer.get(), 0, 64, finalMatrix.constData());
}

bool
RenderEnginePrivate::updateBlitResources(const RenderEngine::Context& context)
{
    if (!context.isValid() || !d.deviceRhi || !d.mvpBuffer || !d.sampler || !d.renderTexture)
        return false;

    d.deviceRenderPassDescriptor = context.renderPassDescriptor;
    d.size = context.size;

    d.blitPipeline.reset();
    d.blitShaderBindings.reset();

    d.blitShaderBindings.reset(d.deviceRhi->newShaderResourceBindings());
    d.blitShaderBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.mvpBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.renderTexture.get(),
                                                  d.sampler.get()),
    });

    if (!d.blitShaderBindings->create()) {
        d.error = core::Error("renderengine", "could not create blit shader bindings");
        d.blitShaderBindings.reset();
        return false;
    }

    d.blitPipeline.reset(d.deviceRhi->newGraphicsPipeline());
    d.blitPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    const QShader vertexShader = compileShader(loadShader("transform"), QShader::VertexStage);
    if (!vertexShader.isValid()) {
        d.error = core::Error("renderengine", "could not compile transform shader");
        d.blitPipeline.reset();
        return false;
    }

    const QShader fragmentShader = compileShader(loadShader("blit"), QShader::FragmentStage);
    if (!fragmentShader.isValid()) {
        d.error = core::Error("renderengine", "could not compile blit shader");
        d.blitPipeline.reset();
        return false;
    }

    d.blitPipeline->setShaderStages(
        { { QRhiShaderStage::Vertex, vertexShader }, { QRhiShaderStage::Fragment, fragmentShader } });
    d.blitPipeline->setVertexInputLayout(d.quadLayout);
    d.blitPipeline->setShaderResourceBindings(d.blitShaderBindings.get());
    d.blitPipeline->setRenderPassDescriptor(d.deviceRenderPassDescriptor);

    if (!d.blitPipeline->create()) {
        d.error = core::Error("renderengine", "could not create blit pipeline");
        d.blitPipeline.reset();
        return false;
    }

    return true;
}

void
RenderEnginePrivate::freeResources()
{
    d.deviceRhi = nullptr;
    d.deviceRenderPassDescriptor = nullptr;
    d.renderTexture.reset();
    d.renderTarget.reset();
    d.renderPassDescriptor.reset();
    d.blitPipeline.reset();
    d.blitShaderBindings.reset();
    d.renderStates.clear();
    d.quadBuffer.reset();
    d.mvpBuffer.reset();
    d.sampler.reset();
    d.quadBufferUploaded = false;
    d.mvpBufferUploaded = false;
    d.valid = false;
    d.frameIndex = 0;
    d.shaderSourceCache.clear();
    d.generatedShaderSourceCache.clear();
    d.shaderCache.clear();
}

void
RenderEnginePrivate::render(const RenderEngine::Context& context, QRhiCommandBuffer* commandBuffer)
{
#if RE_STATS_ENABLED
    QElapsedTimer frameTimer;
    QElapsedTimer timer;
    frameTimer.start();
#endif

    ++d.frameIndex;
    resetFrameStats();

    if (!d.quadBufferUploaded) {
        QRhiResourceUpdateBatch* u = d.deviceRhi->nextResourceUpdateBatch();
        u->updateDynamicBuffer(d.quadBuffer.get(), 0, static_cast<quint32>(d.quad.size() * sizeof(float)),
                               d.quad.data());
        commandBuffer->resourceUpdate(u);
        d.quadBufferUploaded = true;
    }
    if (!d.mvpBufferUploaded) {
        QMatrix4x4 m;
        m.setToIdentity();
        QRhiResourceUpdateBatch* u = d.deviceRhi->nextResourceUpdateBatch();
        u->updateDynamicBuffer(d.mvpBuffer.get(), 0, 64, m.constData());
        commandBuffer->resourceUpdate(u);
        d.mvpBufferUploaded = true;
    }
    QRhiResourceUpdateBatch* resourceUpdates = d.deviceRhi->nextResourceUpdateBatch();

#if RE_STATS_ENABLED
    timer.start();
#endif
    updateRenderStates(context, resourceUpdates);
#if RE_STATS_ENABLED
    d.stats.updateRenderStatesNs = timer.nsecsElapsed();
    timer.restart();
#endif

    updateBlit(context, resourceUpdates);
#if RE_STATS_ENABLED
    d.stats.updateBlitNs = timer.nsecsElapsed();
    timer.restart();
#endif

    commandBuffer->beginPass(d.renderTarget.get(), d.background, { 1.0f, 0 }, resourceUpdates);
    renderScene(context, commandBuffer);
    commandBuffer->endPass();

#if RE_STATS_ENABLED
    d.stats.renderSceneNs = timer.nsecsElapsed();
    timer.restart();
#endif

    commandBuffer->beginPass(context.renderTarget, Qt::transparent, { 1.0f, 0 });
    renderBlit(context, commandBuffer);
    commandBuffer->endPass();

#if RE_STATS_ENABLED
    d.stats.renderBlitNs = timer.nsecsElapsed();
    d.stats.renderFrameNs = frameTimer.nsecsElapsed();
    logFrameStats();

    qDebug() << "\n";
#endif
}

void
RenderEnginePrivate::renderScene(const RenderEngine::Context& renderContext, QRhiCommandBuffer* commandBuffer)
{
    if (!d.quadBuffer)
        return;

    const QSize size = renderContext.size;
    const QSize targetSize = d.renderTexture->pixelSize();
    commandBuffer->setViewport({ 0, 0, float(targetSize.width()), float(targetSize.height()) });

    for (int i = 0; i < d.imageLayers.size(); ++i) {
        if (size_t(i) >= d.renderStates.size())
            continue;

        RenderState& renderState = d.renderStates[size_t(i)];
        if (!renderState.pipeline.get()) {
            continue;
        }

        if (!renderState.shaderBindings)
            continue;

        commandBuffer->setGraphicsPipeline(renderState.pipeline.get());
        commandBuffer->setShaderResources(renderState.shaderBindings.get());

        const QRhiCommandBuffer::VertexInput vbufBinding(d.quadBuffer.get(), 0);
        commandBuffer->setVertexInput(0, 1, &vbufBinding);
        commandBuffer->draw(4);
    }
}

void
RenderEnginePrivate::renderBlit(const RenderEngine::Context& context, QRhiCommandBuffer* commandBuffer)
{
    if (!d.blitPipeline || !d.blitShaderBindings || !d.quadBuffer)
        return;

    commandBuffer->setGraphicsPipeline(d.blitPipeline.get());
    commandBuffer->setShaderResources(d.blitShaderBindings.get());

    const QSize size = context.size;
    commandBuffer->setViewport({ 0, 0, float(size.width()), float(size.height()) });

    const QSize widgetSize = context.size;
    const QSize textureSize = d.renderTexture->pixelSize();

    commandBuffer->setViewport({ 0, 0, float(widgetSize.width()), float(widgetSize.height()) });

    const QRhiCommandBuffer::VertexInput vbufBinding(d.quadBuffer.get(), 0);
    commandBuffer->setVertexInput(0, 1, &vbufBinding);
    commandBuffer->draw(4);
}

QString
RenderEnginePrivate::loadShader(const QString& name)
{
    const auto it = d.shaderSourceCache.constFind(name);
    if (it != d.shaderSourceCache.constEnd()) {
#if RE_STATS_ENABLED
        ++d.stats.shaderSourceCacheHits;
#endif
        RE_TRACE() << "renderengine: shader source cache hit:" << name;
        return it.value();
    }

#if RE_STATS_ENABLED
    ++d.stats.shaderSourceCacheMisses;
#endif

    QFile file(":/flipmansdk/glsl/" + name + ".glsl");
    if (!file.open(QIODevice::ReadOnly)) {
        d.error = core::Error("renderengine", "could not load shader for: " + name);
        return QString();
    }

    const QByteArray bytes = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString source = decoder.decode(bytes);

    if (decoder.hasError()) {
        d.error = core::Error("shaderinterpreter", "shader file is not valid utf-8: " + name);
        return {};
    }

    file.close();

    d.shaderSourceCache.insert(name, source);
    RE_TRACE() << "renderengine: shader source cache miss:" << name;

    return source;
}

QShader
RenderEnginePrivate::compileShader(const QString& source, QShader::Stage stage)
{
    if (!d.deviceRhi)
        return QShader();

    const QRhi::Implementation impl = d.deviceRhi->backend();
    const QByteArray hash = QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString cacheKey = QString("%1:%2:%3").arg(int(impl)).arg(int(stage)).arg(QString::fromLatin1(hash));

    const auto it = d.shaderCache.constFind(cacheKey);
    if (it != d.shaderCache.constEnd()) {
#if RE_STATS_ENABLED
        ++d.stats.shaderCacheHits;
#endif
        RE_TRACE() << "renderengine: shader cache hit:" << cacheKey;
        return it.value();
    }

#if RE_STATS_ENABLED
    ++d.stats.shaderCacheMisses;
#endif
    RE_TRACE() << "renderengine: shader cache miss:" << cacheKey;

    render::ShaderCompiler compiler;
    render::ShaderCompiler::Options opts;
    opts.glslVersion = 440;

    switch (impl) {
    case QRhi::Vulkan: opts.generateSpirv = true; break;

    case QRhi::Metal:
        opts.generateSpirv = true;
        opts.generateMsl = true;
        break;

    case QRhi::D3D11:
    case QRhi::D3D12:
        opts.generateSpirv = true;
        opts.generateHlsl = true;
        break;

    case QRhi::OpenGLES2: opts.generateSpirv = true; break;

    default:
        opts.generateSpirv = true;
        opts.generateMsl = true;
        opts.generateHlsl = true;
        break;
    }

    QShader shader = compiler.compile(source, stage, opts);
    if (!shader.isValid()) {
        d.error = compiler.error();
        return QShader();
    }

    d.shaderCache.insert(cacheKey, shader);
    return shader;
}

QRectF
RenderEnginePrivate::aspectFit(const QSize& src, const QSize& dst)
{
    if (src.isEmpty() || dst.isEmpty())
        return {};
    const float srcAspect = float(src.width()) / float(src.height());
    const float dstAspect = float(dst.width()) / float(dst.height());
    float w, h;
    if (dstAspect > srcAspect) {
        h = float(dst.height());
        w = h * srcAspect;
    }
    else {
        w = float(dst.width());
        h = w / srcAspect;
    }
    const float x = (dst.width() - w) * 0.5f;
    const float y = (dst.height() - h) * 0.5f;
    return QRectF(x, y, w, h);
}

QByteArray
RenderEnginePrivate::shaderHash(const QString& text) const
{
    return QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex();
}

QString
RenderEnginePrivate::buildLayerShaderKey(RenderState::TextureType textureType,
                                         const ShaderDefinition* effectDefinition) const
{
    const QString idtCode =
        R"(vec4 idt(vec4 c)
{
    return pow(max(c, vec4(0.0)), vec4(2.2));
})";

    const QString odtCode =
        R"(vec4 odt(vec4 c)
{
    c = max(c, vec4(0.0));
    return pow(c, vec4(1.0 / 2.2));
})";

    QString effectShaderCode;
    QString effectUniformBlock;

    if (effectDefinition) {
        effectShaderCode = effectDefinition->shaderCode();
        effectUniformBlock = effectDefinition->uniformBlock(3);
    }

    return QString("layer:%1:%2:%3:%4:%5")
        .arg(int(textureType))
        .arg(QString::fromLatin1(shaderHash(idtCode)))
        .arg(QString::fromLatin1(shaderHash(odtCode)))
        .arg(QString::fromLatin1(shaderHash(effectShaderCode)))
        .arg(QString::fromLatin1(shaderHash(effectUniformBlock)));
}

QString
RenderEnginePrivate::buildLayerShaderSource(RenderState::TextureType textureType,
                                            const ShaderDefinition* effectDefinition)
{
    const QString shaderKey = buildLayerShaderKey(textureType, effectDefinition);
    const auto it = d.generatedShaderSourceCache.constFind(shaderKey);
    if (it != d.generatedShaderSourceCache.constEnd()) {
#if RE_STATS_ENABLED
        ++d.stats.generatedShaderSourceCacheHits;
#endif
        RE_TRACE() << "renderengine: generated shader source cache hit:" << shaderKey;
        return it.value();
    }

#if RE_STATS_ENABLED
    ++d.stats.generatedShaderSourceCacheMisses;
#endif
    RE_TRACE() << "renderengine: generated shader source cache miss:" << shaderKey;

    const QString layerSource = loadShader("layer");
    if (layerSource.isEmpty())
        return {};

    ShaderParser shaderParser;
    ShaderContract shaderContract;

    const QString idtCode =
        R"(vec4 idt(vec4 c)
{
    return pow(max(c, vec4(0.0)), vec4(2.2));
})";

    const ShaderDefinition idtDefinition = shaderParser.parse(idtCode);
    if (!shaderParser.isValid()) {
        d.error = core::Error("renderengine", "failed to parse idt shader: " + shaderParser.error().message());
        return {};
    }

    const ShaderContract::Type idtType = ShaderContract::Type::Idt;
    if (!shaderContract.validate(idtType, idtDefinition.functions())) {
        d.error = core::Error("renderengine", "idt shader does not satisfy " + shaderContract.name(idtType));
        return {};
    }

    const QString odtCode =
        R"(vec4 odt(vec4 c)
{
    c = max(c, vec4(0.0));
    return pow(c, vec4(1.0 / 2.2));
})";

    const ShaderDefinition odtDefinition = shaderParser.parse(odtCode);
    if (!shaderParser.isValid()) {
        d.error = core::Error("renderengine", "failed to parse odt shader: " + shaderParser.error().message());
        return {};
    }

    const ShaderContract::Type odtType = ShaderContract::Type::Odt;
    if (!shaderContract.validate(odtType, odtDefinition.functions())) {
        d.error = core::Error("renderengine", "odt shader does not satisfy " + shaderContract.name(odtType));
        return {};
    }

    QString texUniformBlock;
    QString texCall;

    if (textureType == RenderState::TextureType::Nv12) {
        texUniformBlock =
            R"(
layout(binding = 1) uniform sampler2D tex0;
layout(binding = 2) uniform sampler2D tex1;
)";

        texCall =
            R"(
float y = texture(tex0, uv).r;
vec2 uvv = texture(tex1, uv).rg;
vec3 rgb = nv12ToRgb(y, uvv);
vec4 color = vec4(rgb, 1.0);
)";
    }
    else {
        texUniformBlock = R"(layout(binding = 1) uniform sampler2D tex;)";
        texCall = R"(vec4 color = texture(tex, uv);)";
    }

    ShaderParser::Options options;
    options.injections.set("texUniform", texUniformBlock);
    options.injections.set("texCall", texCall);
    options.injections.set("idtCode", idtDefinition.shaderCode());
    options.injections.set("odtCode", odtDefinition.shaderCode());
    options.injections.set("idtCall", shaderContract.call(idtType));
    options.injections.set("odtCall", shaderContract.call(odtType));

    if (effectDefinition) {
        const ShaderContract::Type effectType = ShaderContract::Type::Effect;
        options.injections.set("effectUniform", effectDefinition->uniformBlock(3));
        options.injections.set("effectCode", effectDefinition->shaderCode());
        options.injections.set("effectCall", shaderContract.call(effectType));
    }
    else {
        options.injections.set("effectUniform", QString());
        options.injections.set("effectCode", QString());
        options.injections.set("effectCall", QString());
    }

    ShaderParser layerParser;
    const ShaderDefinition layerDefinition = layerParser.parse(layerSource, options);
    if (!layerParser.isValid()) {
        d.error = core::Error("renderengine", "failed to parse layer shader: " + layerParser.error().message());
        return {};
    }

    const QString generatedSource = layerDefinition.shaderCode();
    d.generatedShaderSourceCache.insert(shaderKey, generatedSource);
    return generatedSource;
}

void
RenderEnginePrivate::resetFrameStats()
{
#if RE_STATS_ENABLED
    d.stats.reset(d.frameIndex, static_cast<int>(d.imageLayers.size()));
#endif
}

void
RenderEnginePrivate::logFrameStats() const
{
#if RE_STATS_ENABLED
    auto ms = [](qint64 ns) { return double(ns) / 1000000.0; };
    qDebug().nospace() << "renderengine: frame " << d.stats.frameIndex << " layers=" << d.stats.layerCount
                       << " frameMs=" << ms(d.stats.renderFrameNs) << " updateMs=" << ms(d.stats.updateRenderStatesNs)
                       << " sceneMs=" << ms(d.stats.renderSceneNs) << " blitMs=" << ms(d.stats.renderBlitNs)
                       << " blitUpdateMs=" << ms(d.stats.updateBlitNs) << " texInit=" << d.stats.texturesInitialized
                       << " texUpload=" << d.stats.textureUploads << " texBytes=" << d.stats.textureUploadBytes
                       << " globalUpd=" << d.stats.globalBufferUpdates
                       << " globalBytes=" << d.stats.globalBufferUploadBytes
                       << " effectCreate=" << d.stats.effectBufferCreates
                       << " effectUpd=" << d.stats.effectBufferUpdates
                       << " effectBytes=" << d.stats.effectBufferUploadBytes
                       << " srbCreate=" << d.stats.shaderBindingsCreated << " pipeCreate=" << d.stats.pipelinesCreated
                       << " srcHit=" << d.stats.shaderSourceCacheHits << " srcMiss=" << d.stats.shaderSourceCacheMisses
                       << " genHit=" << d.stats.generatedShaderSourceCacheHits
                       << " genMiss=" << d.stats.generatedShaderSourceCacheMisses
                       << " shaderHit=" << d.stats.shaderCacheHits << " shaderMiss=" << d.stats.shaderCacheMisses;
#endif
}

RenderEngine::RenderEngine(QObject* parent)
    : QObject(parent)
    , p(new RenderEnginePrivate())
{}

RenderEngine::~RenderEngine() = default;

bool
RenderEngine::initialized() const
{
    return p->d.valid;
}

bool
RenderEngine::initialize(const Context& context)
{
    if (!context.isValid())
        return false;

    const bool deviceChanged = p->d.deviceRhi != context.rhi;

    const bool blitContextChanged = p->d.deviceRenderPassDescriptor != context.renderPassDescriptor
                                    || p->d.size != context.size;

    if (!p->d.valid || deviceChanged) {
        p->freeResources();
        return p->initResources(context);
    }

    if (blitContextChanged) {
        return p->updateBlitResources(context);
    }

    return true;
}

void
RenderEngine::render(const RenderEngine::Context& context, QRhiCommandBuffer* commandBuffer)
{
    if (!p->d.valid || !context.isValid())
        return;

    p->render(context, commandBuffer);
}

QSize
RenderEngine::resolution() const
{
    return p->d.resolution;
}

void
RenderEngine::setResolution(const QSize& resolution)
{
    p->d.resolution = resolution;
}

QColor
RenderEngine::background() const
{
    return p->d.background;
}

void
RenderEngine::setBackground(const QColor& background)
{
    p->d.background = background;
}

void
RenderEngine::setImageLayers(const QList<ImageLayer>& imageLayers)
{
    p->d.imageLayers = imageLayers;
}

bool
RenderEngine::isValid() const
{
    return p->d.valid;
}

void
RenderEngine::reset()
{
    p->freeResources();
    p->d.imageLayers.clear();
}

}  // namespace flipman::sdk::render
