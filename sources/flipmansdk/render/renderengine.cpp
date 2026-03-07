// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderengine.h>

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>

#include <QDebug>
#include <QFile>
#include <QMatrix4x4>

#include <rhi/qshaderbaker.h>

namespace flipman::sdk::render {

class RenderEnginePrivate : public QSharedData {
public:
    RenderEnginePrivate();
    void initResources(const RenderEngine::Context& context);
    void updateImageLayers(const RenderEngine::Context&, QRhiResourceUpdateBatch* updates);
    void updateBlit(const RenderEngine::Context&, QRhiResourceUpdateBatch*);
    void freeResources();
    void render(const RenderEngine::Context& context, QRhiCommandBuffer* commandBuffer);
    void renderScene(const RenderEngine::Context&, QRhiCommandBuffer*);
    void renderBlit(const RenderEngine::Context&, QRhiCommandBuffer*);
    QShader loadShader(const QString& name);
    QShader loadShader(const QByteArray& source, QShader::Stage stage);
    static QRectF aspectFit(const QSize& src, const QSize& dst);

public:
    struct LayerState {
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiShaderResourceBindings> shaderBindings;
        std::unique_ptr<QRhiBuffer> mvpBuffer;
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
        std::unique_ptr<QRhiGraphicsPipeline> layerPipeline;
        std::unique_ptr<QRhiShaderResourceBindings> layerShaderResourceBindings;
        std::vector<LayerState> layerStates;
        std::unique_ptr<QRhiBuffer> quadBuffer;
        std::unique_ptr<QRhiBuffer> mvpBuffer;
        std::unique_ptr<QRhiSampler> sampler;
        std::vector<float> quad;
        QSize size;
        QSize resolution = QSize(1920, 1080);
        QColor background = core::style()->color(core::Style::Viewer);
        QList<ImageLayer> imageLayers;
        bool quadBufferUploaded = false;
        bool mvpBufferUploaded = false;
        bool valid = false;
    };
    Data d;
};

RenderEnginePrivate::RenderEnginePrivate() {}


QByteArray blitSource = R"(

#version 440

layout(location = 2) in vec2 uv_coord;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform sampler2D tex;

void main()
{
    fragColor = texture(tex, uv_coord);
}

)";

QByteArray layerSource = R"(

#version 440

layout(location = 2) in vec2 uv_coord;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform sampler2D tex;

void main()
{
    fragColor = texture(tex, uv_coord);
}

)";

QByteArray quadSource = R"(

#version 440
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) out vec2 uv_coord;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main()
{
    uv_coord = uv;
    gl_Position = mvp * vec4(position, 1.0);
}

)";



void
RenderEnginePrivate::initResources(const RenderEngine::Context& renderContext)
{
    if (!renderContext.isValid())
        return;

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

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 5 * sizeof(float) } });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) },
    });

    d.renderTexture.reset(d.deviceRhi->newTexture(QRhiTexture::RGBA16F, d.resolution, 1,
                                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    d.renderTexture->create();

    QRhiTextureRenderTargetDescription textureRenderTargetDesc(QRhiColorAttachment(d.renderTexture.get()));
    d.renderTarget.reset(d.deviceRhi->newTextureRenderTarget(textureRenderTargetDesc));
    d.renderPassDescriptor.reset(d.renderTarget->newCompatibleRenderPassDescriptor());
    d.renderTarget->setRenderPassDescriptor(d.renderPassDescriptor.get());
    d.renderTarget->create();

    d.layerPipeline.reset(d.deviceRhi->newGraphicsPipeline());
    d.layerPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    d.layerPipeline->setShaderStages({

        { QRhiShaderStage::Vertex, loadShader(quadSource, QShader::Stage::VertexStage) },
        { QRhiShaderStage::Fragment, loadShader(layerSource, QShader::Stage::FragmentStage) },

        //{ QRhiShaderStage::Vertex, loadShader("quad.vert.qsb") },
        //{ QRhiShaderStage::Fragment, loadShader("layer.frag.qsb") },
    });
    d.layerPipeline->setVertexInputLayout(inputLayout);

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    d.layerPipeline->setTargetBlends({ blend });

    d.layerShaderResourceBindings.reset(d.deviceRhi->newShaderResourceBindings());
    d.layerShaderResourceBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.mvpBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                  d.renderTexture.get(),  // placeholder
                                                  d.sampler.get()),
    });
    d.layerShaderResourceBindings->create();
    d.layerPipeline->setShaderResourceBindings(d.layerShaderResourceBindings.get());
    d.layerPipeline->setRenderPassDescriptor(d.renderPassDescriptor.get());
    d.layerPipeline->create();

    d.blitShaderBindings.reset(d.deviceRhi->newShaderResourceBindings());
    d.blitShaderBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.mvpBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.renderTexture.get(),
                                                  d.sampler.get()),
    });
    d.blitShaderBindings->create();

    d.blitPipeline.reset(d.deviceRhi->newGraphicsPipeline());
    d.blitPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    d.blitPipeline->setShaderStages({


        { QRhiShaderStage::Vertex, loadShader(quadSource, QShader::Stage::VertexStage) },
        { QRhiShaderStage::Fragment, loadShader(blitSource, QShader::Stage::FragmentStage) },

        //{ QRhiShaderStage::Vertex, loadShader("quad.vert.qsb") },
        //{ QRhiShaderStage::Fragment, loadShader("blit.frag.qsb") },

    });
    d.blitPipeline->setVertexInputLayout(inputLayout);
    d.blitPipeline->setShaderResourceBindings(d.blitShaderBindings.get());
    d.blitPipeline->setRenderPassDescriptor(d.deviceRenderPassDescriptor);
    d.blitPipeline->create();

    d.quadBufferUploaded = false;
    d.mvpBufferUploaded = false;
    d.valid = true;
    d.layerStates.clear();
}

static QRhiTexture::Format
toRhiFormat(const core::ImageBuffer& image)
{
    using Type = core::ImageFormat::Type;

    const Type type = image.imageFormat().type();
    const int ch = image.channels();

    switch (type) {
    case Type::UInt8:
        if (ch == 1)
            return QRhiTexture::R8;
        if (ch == 2)
            return QRhiTexture::RG8;
        if (ch == 4)
            return QRhiTexture::RGBA8;
        break;

    case Type::Int8:
        if (ch == 1)
            return QRhiTexture::R8SI;
        break;

    case Type::UInt16:
        if (ch == 1)
            return QRhiTexture::R16;
        if (ch == 2)
            return QRhiTexture::RG16;
        break;

    case Type::Half:
        if (ch == 1)
            return QRhiTexture::R16F;
        if (ch == 4)
            return QRhiTexture::RGBA16F;
        break;

    case Type::Float:
        if (ch == 1)
            return QRhiTexture::R32F;
        if (ch == 4)
            return QRhiTexture::RGBA32F;
        break;

    case Type::Int32:
        if (ch == 1)
            return QRhiTexture::R32SI;
        if (ch == 2)
            return QRhiTexture::RG32SI;
        if (ch == 4)
            return QRhiTexture::RGBA32SI;
        break;

    case Type::UInt32:
        if (ch == 1)
            return QRhiTexture::R32UI;
        if (ch == 2)
            return QRhiTexture::RG32UI;
        if (ch == 4)
            return QRhiTexture::RGBA32UI;
        break;

    default: break;
    }

    return QRhiTexture::UnknownFormat;
}

void
RenderEnginePrivate::updateImageLayers(const RenderEngine::Context& context, QRhiResourceUpdateBatch* updates)
{
    if (!updates)
        return;

    if (d.layerStates.size() != size_t(d.imageLayers.size())) {
        d.layerStates.clear();
        d.layerStates.resize(size_t(d.imageLayers.size()));
    }

    const QSize targetSize = d.renderTexture->pixelSize();
    for (int i = 0; i < d.imageLayers.size(); ++i) {
        LayerState& state = d.layerStates[size_t(i)];
        const ImageLayer& layer = d.imageLayers[i];
        const core::ImageBuffer image = layer.image();

        if (!image.isValid())
            continue;

        const QSize texSize(image.dataWindow().width(), image.dataWindow().height());
        const bool sizeChanged = (!state.texture || state.texture->pixelSize() != texSize);
        const bool dataChanged = (state.image.data() != image.data());
        const bool imageChanged = (state.image != image) || dataChanged;

        if (!sizeChanged && !imageChanged && state.shaderBindings) {
            continue;
        }

        if (sizeChanged) {
            state.texture.reset(d.deviceRhi->newTexture(QRhiTexture::RGBA8, texSize, 1, QRhiTexture::Flags()));
            state.texture->create();
            state.shaderBindings.reset();
        }

        QRhiTextureSubresourceUploadDescription subres(image.data(), static_cast<quint32>(image.byteSize()));
        subres.setDataStride(static_cast<quint32>(image.strideSize()));
        subres.setSourceSize(texSize);
        subres.setSourceTopLeft(QPoint(0, 0));
        subres.setDestinationTopLeft(QPoint(0, 0));
        updates->uploadTexture(state.texture.get(),
                               QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, subres) }));

        if (!state.mvpBuffer) {
            state.mvpBuffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
            state.mvpBuffer->create();
        }

        QRectF fit = aspectFit(texSize, targetSize);
        float sx = float(fit.width()) / float(targetSize.width());
        float sy = float(fit.height()) / float(targetSize.height());
        float tx = (fit.x() / float(targetSize.width())) * 2.0f;
        float ty = (fit.y() / float(targetSize.height())) * 2.0f;

        QMatrix4x4 m;
        m.setToIdentity();
        m.translate(-1.0f + sx + tx, -1.0f + sy + ty);
        m.scale(sx, sy);
        updates->updateDynamicBuffer(state.mvpBuffer.get(), 0, 64, m.constData());

        if (!state.shaderBindings) {
            state.shaderBindings.reset(d.deviceRhi->newShaderResourceBindings());
            state.shaderBindings->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0,
                                                         QRhiShaderResourceBinding::VertexStage
                                                             | QRhiShaderResourceBinding::FragmentStage,
                                                         state.mvpBuffer.get()),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                          state.texture.get(), d.sampler.get()),
            });
            state.shaderBindings->create();
        }
        state.image = image;
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
    d.layerPipeline.reset();
    d.layerShaderResourceBindings.reset();
    d.layerStates.clear();
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
    updateImageLayers(context, resourceUpdates);
    updateBlit(context, resourceUpdates);

    commandBuffer->beginPass(d.renderTarget.get(), d.background, { 1.0f, 0 }, resourceUpdates);
    renderScene(context, commandBuffer);
    commandBuffer->endPass();

    commandBuffer->beginPass(context.renderTarget, Qt::transparent, { 1.0f, 0 });
    renderBlit(context, commandBuffer);
    commandBuffer->endPass();
}

static QRectF
aspectFit(const QSize& src, const QSize& dst)
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


void
RenderEnginePrivate::renderScene(const RenderEngine::Context& renderContext, QRhiCommandBuffer* commandBuffer)
{
    if (!d.layerPipeline || !d.quadBuffer)
        return;

    const QSize size = renderContext.size;
    const QSize targetSize = d.renderTexture->pixelSize();
    commandBuffer->setViewport({ 0, 0, float(targetSize.width()), float(targetSize.height()) });

    for (int i = 0; i < d.imageLayers.size(); ++i) {
        if (size_t(i) >= d.layerStates.size())
            continue;

        LayerState& state = d.layerStates[size_t(i)];
        if (!state.shaderBindings)
            continue;

        commandBuffer->setGraphicsPipeline(d.layerPipeline.get());
        commandBuffer->setShaderResources(state.shaderBindings.get());

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

QShader
RenderEnginePrivate::loadShader(const QString& name)
{
    QFile file(QStringLiteral(":/shaders/render/") + name);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "failed to load shader" << name;
        return QShader();
    }
    return QShader::fromSerialized(file.readAll());
}

QShader
RenderEnginePrivate::loadShader(const QByteArray& source, QShader::Stage stage)
{
    QShaderBaker baker;
    baker.setSourceString(source, stage);
    baker.setGeneratedShaders({ { QShader::SpirvShader, QShaderVersion(100) },
                                { QShader::GlslShader, QShaderVersion(100) },
                                { QShader::HlslShader, QShaderVersion(50) },
                                { QShader::MslShader, QShaderVersion(12) } });
    baker.setGeneratedShaderVariants({ QShader::StandardShader, QShader::BatchableVertexShader });
    baker.setSpirvOptions(QShaderBaker::SpirvOption::StripDebugAndVarInfo);

    QShader shader = baker.bake();
    if (!shader.isValid()) {
        qWarning() << "shader bake failed:" << baker.errorMessage();
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

void
RenderEngine::initialize(const RenderEngine::Context& context)
{
    if (!context.isValid())
        return;

    if (p->d.deviceRhi != context.rhi || p->d.deviceRenderPassDescriptor != context.renderPassDescriptor
        || p->d.size != context.size) {
        p->freeResources();
        p->initResources(context);
    }
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
