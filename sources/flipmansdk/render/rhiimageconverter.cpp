// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "private/pixelsampling.h"
#include "private/rhiimageconverter.h"

#include <flipmansdk/core/profile.h>
#include <QThread>
#include <rhi/qshaderbaker.h>
#include <array>
#include <limits>
#include <memory>

namespace flipman::sdk::render {

namespace {

    const char* vertexSource = R"(#version 440
void main() {
    vec2 p = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";
    const QByteArray bufferSource = QByteArray(R"(#version 440
layout(std140, binding=0) uniform Params { ivec4 geometry; ivec4 format; } params;
layout(location=0) out vec4 fragColor;
)") + pixelBufferSampling + R"(
void main()
{
    fragColor = fmImagePixel(ivec2(gl_FragCoord.xy), params.geometry, params.format.x, params.format.y != 0);
}
)";
    const char* textureSource = R"(#version 440
layout(binding=1) uniform sampler2D sourceTexture;
layout(location=0) out vec4 fragColor;
void main() { fragColor = texelFetch(sourceTexture, ivec2(gl_FragCoord.xy), 0); }
)";

    bool supported(QRhiTexture::Format format)
    {
        return format == QRhiTexture::RGBA8 || format == QRhiTexture::BGRA8 || format == QRhiTexture::RGBA16F
               || format == QRhiTexture::RGBA32F;
    }

    QShader bake(const char* text, QShader::Stage stage, QString& error)
    {
        QShaderBaker baker;
        baker.setSourceString(QByteArray(text), stage);
        baker.setGeneratedShaderVariants({ QShader::StandardShader });
        baker.setGeneratedShaders({ { QShader::SpirvShader, QShaderVersion(100) },
                                    { QShader::MslShader, QShaderVersion(12) },
                                    { QShader::HlslShader, QShaderVersion(50) },
                                    { QShader::GlslShader, QShaderVersion(440) } });
        const auto result = baker.bake();
        if (!result.isValid())
            error = baker.errorMessage();
        return result;
    }

}  // namespace

class RhiImageConverterPrivate {
public:
    bool fail(const QString& message);
    bool validate(QRhiCommandBuffer* commands, const QSize& size, QRhiTextureRenderTarget* target);
    bool prepare(QRhiTextureRenderTarget* target, QRhiTexture* source, int bytes);
    void draw(QRhiCommandBuffer* commands, QRhiTextureRenderTarget* target, QRhiResourceUpdateBatch* updates);

    struct Data {
        QRhi* rhi = nullptr;
        QRhiTexture* source = nullptr;
        core::Error error;
        QThread* thread = nullptr;
        QShader vertex;
        QShader fragment;
        std::unique_ptr<QRhiBuffer> uniforms;
        std::unique_ptr<QRhiBuffer> pixels;
        std::unique_ptr<QRhiSampler> sampler;
        std::unique_ptr<QRhiShaderResourceBindings> bindings;
        std::unique_ptr<QRhiRenderPassDescriptor> pass;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        bool textureMode = false;
        bool initialized = false;
    };
    Data d;
};

bool
RhiImageConverterPrivate::fail(const QString& message)
{
    d.error = core::Error("imageconverter", message);
    return false;
}

bool
RhiImageConverterPrivate::validate(QRhiCommandBuffer* commands, const QSize& size, QRhiTextureRenderTarget* target)
{
    if (!d.rhi || QThread::currentThread() != d.thread || !commands || commands->rhi() != d.rhi || !target
        || target->rhi() != d.rhi || !target->renderPassDescriptor() || target->sampleCount() != 1 || size.isEmpty()
        || target->pixelSize() != size || target->description().colorAttachmentCount() != 1)
        return fail("use a matching, single-color target on the device's recording thread");

    const auto* texture = target->description().colorAttachmentAt(0)->texture();
    if (!texture || !supported(texture->format()) || texture->flags().testFlag(QRhiTexture::sRGB)
        || texture->writeViewFormat().srgb || texture->flags().testFlag(QRhiTexture::CubeMap)
        || texture->flags().testFlag(QRhiTexture::TextureArray)
        || texture->flags().testFlag(QRhiTexture::ThreeDimensional))
        return fail("destination must be a non-sRGB RGBA8/BGRA8/RGBA16F/RGBA32F texture");

    d.error = {};
    return true;
}

bool
RhiImageConverterPrivate::prepare(QRhiTextureRenderTarget* target, QRhiTexture* source, int bytes)
{
    const bool textureMode = source != nullptr;
    bool rebind = !d.bindings || source != d.source || textureMode != d.textureMode;
    if (!textureMode && (!d.pixels || d.pixels->size() < bytes)) {
        d.pixels.reset(d.rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, (bytes + 3) & ~3));
        if (!d.pixels->create()) {
            d.pixels.reset();
            return fail("could not create source pixel buffer");
        }

        rebind = true;
    }
    if (!d.uniforms) {
        d.uniforms.reset(d.rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
        d.sampler.reset(d.rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        if (!d.uniforms->create() || !d.sampler->create()) {
            d.uniforms.reset();
            d.sampler.reset();
            return fail("could not create conversion resources");
        }
    }
    if (rebind) {
        // Drop resources referencing the old binding layout before replacing it.
        d.pipeline.reset();
        d.bindings.reset(d.rhi->newShaderResourceBindings());
        if (textureMode)
            d.bindings->setBindings(
                { QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, source,
                                                            d.sampler.get()) });
        else
            d.bindings->setBindings(
                { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::FragmentStage,
                                                           d.uniforms.get()),
                  QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::FragmentStage, d.pixels.get()) });
        if (!d.bindings->create()) {
            d.bindings.reset();
            return fail("could not create conversion bindings");
        }
    }
    if (!d.initialized || textureMode != d.textureMode) {
        QString error;
        if (!d.vertex.isValid())
            d.vertex = bake(vertexSource, QShader::VertexStage, error);
        d.fragment = bake(textureMode ? textureSource : bufferSource.constData(), QShader::FragmentStage, error);
        if (!d.vertex.isValid() || !d.fragment.isValid())
            return fail("shader compilation: " + error);
    }
    d.source = source;
    d.textureMode = textureMode;
    d.initialized = true;
    if (d.pass && !d.pass->isCompatible(target->renderPassDescriptor()))
        d.pipeline.reset();
    if (!d.pipeline) {
        d.pass.reset(target->newCompatibleRenderPassDescriptor());
        d.pipeline.reset(d.rhi->newGraphicsPipeline());
        d.pipeline->setShaderStages(
            { { QRhiShaderStage::Vertex, d.vertex }, { QRhiShaderStage::Fragment, d.fragment } });
        d.pipeline->setShaderResourceBindings(d.bindings.get());
        d.pipeline->setRenderPassDescriptor(d.pass.get());
        if (!d.pipeline->create()) {
            d.pipeline.reset();
            return fail("could not create conversion pipeline");
        }
    }
    return true;
}

void
RhiImageConverterPrivate::draw(QRhiCommandBuffer* commands, QRhiTextureRenderTarget* target,
                               QRhiResourceUpdateBatch* updates)
{
    commands->beginPass(target, Qt::transparent, { 1.0f, 0 }, updates);
    commands->setGraphicsPipeline(d.pipeline.get());
    commands->setViewport(QRhiViewport(0, 0, float(target->pixelSize().width()), float(target->pixelSize().height())));
    commands->setShaderResources(d.bindings.get());
    commands->draw(3);
    commands->endPass();
}

RhiImageConverter::RhiImageConverter(QRhi* rhi)
    : p(new RhiImageConverterPrivate)
{
    p->d.rhi = rhi;
    p->d.thread = rhi ? rhi->thread() : QThread::currentThread();
}

RhiImageConverter::~RhiImageConverter() = default;

bool
RhiImageConverter::convert(QRhiCommandBuffer* commands, const core::ImageBuffer& source,
                           QRhiTextureRenderTarget* target)
{
    FM_PROFILE_SCOPE("image", "gpu.convert_upload");
    if (!p->validate(commands, source.dataWindow().size(), target))
        return false;

    const auto type = source.imageFormat().type();
    const auto layout = source.pixelLayout();
    if (!source.isAllocated() || source.packing() != core::ImageBuffer::Packing::Interleaved
        || source.subsampling() != core::ImageBuffer::Subsampling::None
        || (source.channels() != 3 && source.channels() != 4)
        || (type != core::ImageFormat::UInt8 && type != core::ImageFormat::Half && type != core::ImageFormat::Float)
        || (layout != core::ImageBuffer::PixelLayout::RGB && layout != core::ImageBuffer::PixelLayout::RGBA
            && layout != core::ImageBuffer::PixelLayout::BGRA)
        || (source.channels() == 3 && layout != core::ImageBuffer::PixelLayout::RGB)
        || source.byteSize() > size_t(std::numeric_limits<int>::max() - 3))
        return p->fail("source must be allocated interleaved RGB/RGBA/BGRA UInt8/Half/Float, below 2 GiB");

    if (!p->prepare(target, nullptr, int(source.byteSize())))
        return false;

    const std::array<qint32, 8> params = { source.dataWindow().width(),
                                           source.dataWindow().height(),
                                           qint32(source.strideSize()),
                                           source.channels(),
                                           type == core::ImageFormat::UInt8 ? 0
                                                                            : (type == core::ImageFormat::Half ? 1 : 2),
                                           layout == core::ImageBuffer::PixelLayout::BGRA ? 1 : 0,
                                           0,
                                           0 };
    static_assert(sizeof(params) == 32, "Params must match the GLSL std140 block");
    auto* updates = p->d.rhi->nextResourceUpdateBatch();
    updates->updateDynamicBuffer(p->d.uniforms.get(), 0, sizeof(params), params.data());
    updates->uploadStaticBuffer(p->d.pixels.get(), 0, quint32(source.byteSize()), source.data());
    p->draw(commands, target, updates);
    return true;
}

bool
RhiImageConverter::convert(QRhiCommandBuffer* commands, QRhiTexture* source, QRhiTextureRenderTarget* target)
{
    FM_PROFILE_SCOPE("image", "gpu.convert_texture");
    if (!source || !p->validate(commands, source->pixelSize(), target))
        return p->fail("source texture or target is invalid");

    if (source->rhi() != p->d.rhi || !supported(source->format()) || source->sampleCount() != 1
        || source->flags().testFlag(QRhiTexture::sRGB) || source->readViewFormat().srgb
        || source->flags().testFlag(QRhiTexture::CubeMap) || source->flags().testFlag(QRhiTexture::TextureArray)
        || source->flags().testFlag(QRhiTexture::ThreeDimensional)
        || source == target->description().colorAttachmentAt(0)->texture())
        return p->fail("source must be a distinct, non-sRGB RGBA texture on this device");

    if (!p->prepare(target, source, 0))
        return false;

    p->draw(commands, target, nullptr);
    return true;
}

void
RhiImageConverter::releaseResources()
{
    if (QThread::currentThread() != p->d.thread) {
        p->fail("release conversion resources on the owning thread");
        return;
    }
    p->d.pipeline.reset();
    p->d.pass.reset();
    p->d.bindings.reset();
    p->d.sampler.reset();
    p->d.pixels.reset();
    p->d.uniforms.reset();
    p->d.source = nullptr;
}

core::Error
RhiImageConverter::error() const
{
    return p->d.error;
}

}  // namespace flipman::sdk::render
