// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderengine.h>

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>
#include <flipmansdk/render/shadercompiler.h>
#include <flipmansdk/render/shadercomposer.h>

#include <QDebug>
#include <QFile>
#include <QMatrix4x4>

namespace flipman::sdk::render {

class RenderEnginePrivate : public QSharedData {
public:
    RenderEnginePrivate();
    bool initResources(const RenderEngine::Context& context);
    void updateRenderStates(const RenderEngine::Context&, QRhiResourceUpdateBatch* updates);
    void updateBlit(const RenderEngine::Context&, QRhiResourceUpdateBatch*);
    void freeResources();
    void render(const RenderEngine::Context& context, QRhiCommandBuffer* commandBuffer);
    void renderScene(const RenderEngine::Context&, QRhiCommandBuffer*);
    void renderBlit(const RenderEngine::Context&, QRhiCommandBuffer*);
    QString loadShader(const QString& name);
    QShader compileShader(const QString& source, QShader::Stage stage);
    QRectF aspectFit(const QSize& src, const QSize& dst);

public:
    struct RenderState {
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiShaderResourceBindings> shaderBindings;
        std::unique_ptr<QRhiBuffer> mvpBuffer;
        std::unique_ptr<QRhiBuffer> paramBuffer;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        render::ShaderDefinition shaderDefinition;
        core::ImageBuffer image;
    };
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
    d.quadBuffer->create();

    d.mvpBuffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(float) * 16));
    d.mvpBuffer->create();

    d.sampler.reset(d.deviceRhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                            QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    d.sampler->create();

    d.quadLayout.setBindings({ { 5 * sizeof(float) } });
    d.quadLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) },
    });

    d.renderTexture.reset(d.deviceRhi->newTexture(QRhiTexture::RGBA16F, d.resolution, 1,
                                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!d.renderTexture->create()) {
        d.error = core::Error("renderengine", "could not create render texture");
        return false;
    }

    QRhiTextureRenderTargetDescription textureRenderTargetDesc(QRhiColorAttachment(d.renderTexture.get()));
    d.renderTarget.reset(d.deviceRhi->newTextureRenderTarget(textureRenderTargetDesc));
    d.renderPassDescriptor.reset(d.renderTarget->newCompatibleRenderPassDescriptor());
    d.renderTarget->setRenderPassDescriptor(d.renderPassDescriptor.get());
    if (!d.renderTarget->create()) {
        d.error = core::Error("renderengine", "could not create render target");
        return false;
    }

    d.blitShaderBindings.reset(d.deviceRhi->newShaderResourceBindings());
    d.blitShaderBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.mvpBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.renderTexture.get(),
                                                  d.sampler.get()),
    });
    if (!d.blitShaderBindings->create()) {
        d.error = core::Error("renderengine", "could not create blit shader bindings");
        return false;
    }

    d.blitPipeline.reset(d.deviceRhi->newGraphicsPipeline());
    d.blitPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    QShader vertexShader = compileShader(loadShader("transform"), QShader::VertexStage);
    if (!vertexShader.isValid()) {
        d.error = core::Error("renderengine", "could not compile transform shader");
        return false;
    }

    QShader fragmentShader = compileShader(loadShader("blit"), QShader::FragmentStage);
    if (!fragmentShader.isValid()) {
        d.error = core::Error("renderengine", "could not compile blit shader");
        return false;
    }

    d.blitPipeline->setShaderStages(
        { { QRhiShaderStage::Vertex, vertexShader }, { QRhiShaderStage::Fragment, fragmentShader } });
    d.blitPipeline->setVertexInputLayout(d.quadLayout);
    d.blitPipeline->setShaderResourceBindings(d.blitShaderBindings.get());
    d.blitPipeline->setRenderPassDescriptor(d.deviceRenderPassDescriptor);
    if (!d.blitPipeline->create()) {
        d.error = core::Error("renderengine", "could not create blit pipeline");
        return false;
    }

    d.renderStates.clear();
    d.quadBufferUploaded = false;
    d.mvpBufferUploaded = false;
    d.valid = true;
    return true;
}

void
RenderEnginePrivate::updateRenderStates(const RenderEngine::Context& context, QRhiResourceUpdateBatch* updates)
{
    if (!updates)
        return;

    if (d.renderStates.size() != size_t(d.imageLayers.size())) {
        d.renderStates.clear();
        d.renderStates.resize(size_t(d.imageLayers.size()));
    }

    const QSize targetSize = d.renderTexture->pixelSize();

    for (int i = 0; i < d.imageLayers.size(); ++i) {
        RenderState& renderState = d.renderStates[size_t(i)];
        const ImageLayer& imageLayer = d.imageLayers[i];
        const core::ImageBuffer image = imageLayer.image();

        render::ImageEffect imageEffect = imageLayer.imageEffect();

        ShaderDefinition shaderDefinition;
        const bool hasEffect = imageEffect.isValid();

        if (hasEffect)
            shaderDefinition = imageEffect.shaderDefinition();

        if (!image.isValid())
            continue;

        const QSize texSize(image.dataWindow().width(), image.dataWindow().height());
        const bool sizeChanged = (!renderState.texture || renderState.texture->pixelSize() != texSize);
        const bool dataChanged = (renderState.image.data() != image.data());
        const bool imageChanged = (renderState.image != image) || dataChanged;

        if (sizeChanged) {
            renderState.texture.reset(d.deviceRhi->newTexture(QRhiTexture::RGBA16F, texSize, 1, QRhiTexture::Flags()));

            if (!renderState.texture->create()) {
                qDebug() << "failed to create texture for layer" << i;
                continue;
            }

            renderState.shaderBindings.reset();
            renderState.pipeline.reset();
        }

        if (!renderState.mvpBuffer) {
            renderState.mvpBuffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));

            if (!renderState.mvpBuffer->create()) {
                qDebug() << "failed to create MVP buffer for layer" << i;
                continue;
            }
            renderState.shaderBindings.reset();
            renderState.pipeline.reset();
        }

        {
            bool wantsParamBuffer = false;
            int paramBufferSize = 0;

            if (hasEffect) {
                const auto& params = shaderDefinition.descriptor().parameters;
                if (!params.isEmpty()) {
                    wantsParamBuffer = true;
                    paramBufferSize = params.size() * 16;  // std140-style float-per-slot packing
                }
            }

            if (wantsParamBuffer) {
                const bool needNewParamBuffer = (!renderState.paramBuffer
                                                 || int(renderState.paramBuffer->size()) != paramBufferSize);

                if (needNewParamBuffer) {
                    qDebug() << "creating parameter buffer for layer" << i << "with" << (paramBufferSize / 16)
                             << "params";

                    renderState.paramBuffer.reset(
                        d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, paramBufferSize));

                    if (!renderState.paramBuffer->create()) {
                        qDebug() << "failed to create parameter buffer for layer" << i;
                        continue;
                    }

                    renderState.shaderBindings.reset();
                    renderState.pipeline.reset();
                }
            }
            else if (renderState.paramBuffer) {
                renderState.paramBuffer.reset();
                renderState.shaderBindings.reset();
                renderState.pipeline.reset();
            }
        }

        if (sizeChanged || imageChanged) {
            // todo: HALF16 RGB => RGBA expansion

            const int width = texSize.width();
            const int height = texSize.height();

            const uint16_t* src = reinterpret_cast<const uint16_t*>(image.data());

            // temporary staging buffer
            QByteArray expanded;
            expanded.resize(width * height * 8);  // 4 half channels

            uint16_t* dst = reinterpret_cast<uint16_t*>(expanded.data());

            const size_t srcStride = image.strideSize() / sizeof(uint16_t);  // half units
            const size_t dstStride = width * 4;

            for (int y = 0; y < height; ++y) {
                const uint16_t* srcRow = src + y * srcStride;
                uint16_t* dstRow = dst + y * dstStride;

                for (int x = 0; x < width; ++x) {
                    dstRow[x * 4 + 0] = srcRow[x * 3 + 0];
                    dstRow[x * 4 + 1] = srcRow[x * 3 + 1];
                    dstRow[x * 4 + 2] = srcRow[x * 3 + 2];

                    // alpha = 1.0 (half float)
                    dstRow[x * 4 + 3] = 0x3C00;
                }
            }

            // todo: HALF16 RGB => RGBA expansion

            QRhiTextureSubresourceUploadDescription subres(expanded.constData(), static_cast<quint32>(expanded.size()));

            subres.setDataStride(width * 8);  // RGBA16F stride
            subres.setSourceSize(texSize);
            subres.setSourceTopLeft(QPoint(0, 0));
            subres.setDestinationTopLeft(QPoint(0, 0));

            updates->uploadTexture(renderState.texture.get(),
                                   QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, subres) }));
        }

        {
            const QRectF fit = aspectFit(texSize, targetSize);

            const float sx = float(fit.width()) / float(targetSize.width());
            const float sy = float(fit.height()) / float(targetSize.height());
            const float tx = (fit.x() / float(targetSize.width())) * 2.0f;
            const float ty = (fit.y() / float(targetSize.height())) * 2.0f;

            QMatrix4x4 m;
            m.setToIdentity();
            m.translate(-1.0f + sx + tx, -1.0f + sy + ty);
            m.scale(sx, sy);

            updates->updateDynamicBuffer(renderState.mvpBuffer.get(), 0, 64, m.constData());
        }

        if (renderState.paramBuffer && hasEffect) {
            const auto& params = shaderDefinition.descriptor().parameters;
            QByteArray paramData(params.size() * 16, 0);

            for (int p = 0; p < params.size(); ++p) {
                const auto& param = params[p];
                const float value = param.defaultValue.toFloat();

                memcpy(paramData.data() + p * 16, &value, sizeof(float));
            }
            updates->updateDynamicBuffer(renderState.paramBuffer.get(), 0, static_cast<quint32>(paramData.size()),
                                         paramData.constData());
        }

        if (!renderState.shaderBindings) {
            QVector<QRhiShaderResourceBinding> bindings;

            bindings << QRhiShaderResourceBinding::uniformBuffer(0,
                                                                 QRhiShaderResourceBinding::VertexStage
                                                                     | QRhiShaderResourceBinding::FragmentStage,
                                                                 renderState.mvpBuffer.get());

            bindings << QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                                  renderState.texture.get(), d.sampler.get());

            if (renderState.paramBuffer) {
                qDebug() << "adding EffectParams binding (binding=2)";

                bindings << QRhiShaderResourceBinding::uniformBuffer(2, QRhiShaderResourceBinding::FragmentStage,
                                                                     renderState.paramBuffer.get());
            }

            renderState.shaderBindings.reset(d.deviceRhi->newShaderResourceBindings());
            renderState.shaderBindings->setBindings(bindings.begin(), bindings.end());

            if (!renderState.shaderBindings->create()) {
                qDebug() << "failed to create shader bindings for layer" << i;
                renderState.shaderBindings.reset();
                continue;
            }
        }

        if (!renderState.pipeline) {
            qDebug() << "creating pipeline for layer" << i;

            renderState.pipeline.reset(d.deviceRhi->newGraphicsPipeline());
            renderState.pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

            const QString shaderCode = loadShader("layer");

            const QString idtCode = R"(vec3 idt(vec3 c)
{
    return pow(max(c, vec3(0.0)), vec3(2.2));
})";

            const QString odtCode = R"(vec3 odt(vec3 c)
{
    c = max(c, vec3(0.0));
    return pow(c, vec3(1.0 / 2.2));
})";

            ShaderDefinition layerDefinition;
            {
                ShaderComposer::Options options;

                if (hasEffect) {
                    options.injections.set("effectUniforms", shaderDefinition.uniformBlock());
                    options.injections.set("effectCode", shaderDefinition.shaderCode());
                    options.injections.set("effectApply", shaderDefinition.applyCode());
                    options.injections.set("idtCode", idtCode);
                    options.injections.set("odtCode", odtCode);
                    options.injections.set("idtApply", "color.rgb = idt(color.rgb);");
                    options.injections.set("odtApply", "color.rgb = odt(color.rgb);");
                }
                else {
                    options.injections.set("effectUniforms", "");
                    options.injections.set("effectCode", "");
                    options.injections.set("effectApply", "");
                    options.injections.set("idtCode", idtCode);
                    options.injections.set("odtCode", odtCode);
                    options.injections.set("idtApply", "");
                    options.injections.set("odtApply", "");
                }

                ShaderComposer composer;
                layerDefinition = composer.fromSource(shaderCode, options);
            }

            if (!layerDefinition.isValid()) {
                qDebug() << "shader composition failed";
                d.error = layerDefinition.error();
                qDebug() << "error:" << d.error;
                renderState.pipeline.reset();
                continue;
            }

            qDebug().noquote() << "composed shader:\n" << layerDefinition.shaderCode();

            const QShader fragmentShader = compileShader(layerDefinition.shaderCode(), QShader::FragmentStage);

            if (!fragmentShader.isValid()) {
                qDebug().noquote() << "fragment shader compilation failed:" << d.error.message();
                renderState.pipeline.reset();
                continue;
            }

            const QString vertexSource = loadShader("transform");
            const QShader vertexShader = compileShader(vertexSource, QShader::VertexStage);

            if (!vertexShader.isValid()) {
                qDebug() << "vertex shader compilation failed";
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
                qDebug() << "failed to create pipeline for layer" << i;
                renderState.pipeline.reset();
                continue;
            }
        }
        renderState.image = image;
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
}

void
RenderEnginePrivate::render(const RenderEngine::Context& context, QRhiCommandBuffer* commandBuffer)
{
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
    updateRenderStates(context, resourceUpdates);
    updateBlit(context, resourceUpdates);

    commandBuffer->beginPass(d.renderTarget.get(), d.background, { 1.0f, 0 }, resourceUpdates);
    renderScene(context, commandBuffer);
    commandBuffer->endPass();

    commandBuffer->beginPass(context.renderTarget, Qt::transparent, { 1.0f, 0 });
    renderBlit(context, commandBuffer);
    commandBuffer->endPass();
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

    QRectF vp = aspectFit(textureSize, widgetSize);

    commandBuffer->setViewport({ 0, 0, float(widgetSize.width()), float(widgetSize.height()) });

    const QRhiCommandBuffer::VertexInput vbufBinding(d.quadBuffer.get(), 0);
    commandBuffer->setVertexInput(0, 1, &vbufBinding);
    commandBuffer->draw(4);
}

QString
RenderEnginePrivate::loadShader(const QString& name)
{
    QFile file(":/flipmansdk/glsl/" + name + ".glsl");
    if (!file.open(QIODevice::ReadOnly)) {
        d.error = core::Error("renderengine", "could not load shader for: " + name);
        return QString();
    }
    QByteArray bytes = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    QString source = decoder.decode(bytes);
    if (decoder.hasError()) {
        d.error = core::Error("shaderinterpreter", "shader file is not valid utf-8: " + name);
        return {};
    }
    file.close();
    return source;
}

QShader
RenderEnginePrivate::compileShader(const QString& source, QShader::Stage stage)
{
    render::ShaderCompiler compiler;
    render::ShaderCompiler::Options opts;
    opts.glslVersion = 440;
    const QRhi::Implementation impl = d.deviceRhi->backend();
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

    const bool contextChanged = p->d.deviceRhi != context.rhi
                                || p->d.deviceRenderPassDescriptor != context.renderPassDescriptor
                                || p->d.size != context.size;

    if (!p->d.valid || contextChanged) {
        p->freeResources();
        return p->initResources(context);
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
