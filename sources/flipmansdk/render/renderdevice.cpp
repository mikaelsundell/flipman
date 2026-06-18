// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderdevice.h>

namespace flipman::sdk::render {

class RenderDevicePrivate {
public:
    bool create(RenderDevice::Backend backend, const QSize& size, RenderDevice::TargetFormat renderTargetFormat);
    bool beginFrame(QRhiCommandBuffer*& commandBuffer);
    void endFrame();
    RenderContext context() const;
    RenderSurface surface() const;
    QImage readback() const;
    QRhi::Implementation toBackend(RenderDevice::Backend backend);
    struct Data {
        std::unique_ptr<QRhi> rhi;
        std::unique_ptr<QRhiTexture> colorTexture;
        std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
        std::unique_ptr<QRhiRenderPassDescriptor> renderPassDesc;
        QRhiCommandBuffer* currentCb = nullptr;
        QRhiReadbackResult readResult;
        QSize size;
        QColor clearColor = Qt::black;
        RenderDevice::Backend backend = RenderDevice::Null;
        RenderDevice::TargetFormat targetFormat = RenderDevice::TargetFormat::Rgba16F;
        core::Error error;
        bool initialized = false;
    };
    Data d;
};

QRhi::Implementation
RenderDevicePrivate::toBackend(RenderDevice::Backend backend)
{
    switch (backend) {
    case RenderDevice::Auto:
#if defined(Q_OS_MACOS)
        return QRhi::Metal;
#elif defined(Q_OS_WIN)
        return QRhi::D3D11;
#elif QT_CONFIG(vulkan)
        return QRhi::Vulkan;
#else
        return QRhi::OpenGLES2;
#endif
    case RenderDevice::Metal: return QRhi::Metal;
    case RenderDevice::Vulkan: return QRhi::Vulkan;
    case RenderDevice::Direct3D: return QRhi::D3D11;
    case RenderDevice::OpenGL: return QRhi::OpenGLES2;
    case RenderDevice::Null: return QRhi::Null;
    }
    return QRhi::Null;
}

bool
RenderDevicePrivate::create(RenderDevice::Backend backend, const QSize& size, RenderDevice::TargetFormat targetFormat)
{
    if (size.isEmpty()) {
        d.error = core::Error("renderdevice", "create failed: invalid size");
        return false;
    }
    d.size = size;
    d.backend = backend;
    d.targetFormat = targetFormat;

    const QRhi::Implementation rhiBackend = toBackend(backend);
    QRhiInitParams* params = nullptr;
    QRhiNullInitParams nullParams;
#if QT_CONFIG(metal)
    QRhiMetalInitParams metalParams;
#endif
#if QT_CONFIG(vulkan)
    QRhiVulkanInitParams vkParams;
#endif
#if QT_CONFIG(opengl)
    QRhiGles2InitParams glParams;
#endif

#if defined(Q_OS_WIN)
    QRhiD3D11InitParams d3dParams;
#endif
    switch (rhiBackend) {
    case QRhi::Null: params = &nullParams; break;
#if QT_CONFIG(metal)
    case QRhi::Metal: params = &metalParams; break;
#endif
#if QT_CONFIG(vulkan)
    case QRhi::Vulkan: params = &vkParams; break;
#endif
#if defined(Q_OS_WIN)
    case QRhi::D3D11: params = &d3dParams; break;
#endif
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2: params = &glParams; break;
#endif
    default: params = &nullParams; break;
    }
#if defined(Q_OS_MACOS)
#    if QT_CONFIG(metal)
    backend = RenderDevice::Metal;
#    else
    backend = RenderDevice::Null;
    d.error = core::Error("renderdevice", "metal backend is not available in this Qt build");
    return false;
#    endif
#endif
    d.rhi.reset(QRhi::create(rhiBackend, params));
    if (!d.rhi) {
        d.error = core::Error("renderdevice", "failed to create rhi backend");
        return false;
    }
    QRhiTexture::Format texFormat = QRhiTexture::RGBA16F;
    switch (targetFormat) {
    case RenderDevice::Rgba8: texFormat = QRhiTexture::RGBA8; break;
    case RenderDevice::Rgba16F: texFormat = QRhiTexture::RGBA16F; break;
    case RenderDevice::Rgba32F: texFormat = QRhiTexture::RGBA32F; break;
    }

    d.colorTexture.reset(
        d.rhi->newTexture(texFormat, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));

    if (!d.colorTexture->create()) {
        d.error = core::Error("renderdevice", "failed to create color texture");
        return false;
    }

    QRhiTextureRenderTargetDescription rtDesc(QRhiColorAttachment(d.colorTexture.get()));
    d.renderTarget.reset(d.rhi->newTextureRenderTarget(rtDesc));
    d.renderPassDesc.reset(d.renderTarget->newCompatibleRenderPassDescriptor());
    d.renderTarget->setRenderPassDescriptor(d.renderPassDesc.get());

    if (!d.renderTarget->create()) {
        d.error = core::Error("renderdevice", "failed to create render target");
        return false;
    }

    d.initialized = true;
    return true;
}

bool
RenderDevicePrivate::beginFrame(QRhiCommandBuffer*& commandBuffer)
{
    if (!d.initialized)
        return false;

    if (d.rhi->beginOffscreenFrame(&d.currentCb) != QRhi::FrameOpSuccess)
        return false;

    commandBuffer = d.currentCb;
    return true;
}

void
RenderDevicePrivate::endFrame()
{
    if (!d.initialized || !d.currentCb)
        return;

    d.readResult = {};
    QRhiReadbackDescription desc(d.colorTexture.get());
    auto batch = d.rhi->nextResourceUpdateBatch();
    batch->readBackTexture(desc, &d.readResult);

    d.currentCb->resourceUpdate(batch);
    d.rhi->endOffscreenFrame();
    d.rhi->finish();  // ensure GPU completion

    d.currentCb = nullptr;
}

RenderContext
RenderDevicePrivate::context() const
{
    RenderContext context;

    if (!d.initialized)
        return context;

    RenderSurface surface;
    surface.setRenderTarget(d.renderTarget.get());

    context.setRhi(d.rhi.get());
    context.setSurface(surface);

    return context;
}

RenderDevice::RenderDevice(QObject* parent)
    : QObject(parent)
    , p(new RenderDevicePrivate())
{}

RenderDevice::~RenderDevice() {}

bool
RenderDevice::create(Backend backend, const QSize& size, TargetFormat targetFormat)
{
    return p->create(backend, size, targetFormat);
}

bool
RenderDevice::beginFrame(QRhiCommandBuffer*& commandBuffer)
{
    return p->beginFrame(commandBuffer);
}

void
RenderDevice::endFrame()
{
    return p->endFrame();
}

RenderContext
RenderDevice::context() const
{
    return p->context();
}

RenderDevice::TargetFormat
RenderDevice::targetFormat() const
{
    return p->d.targetFormat;
}

RenderDevice::Backend
RenderDevice::backend() const
{
    return p->d.backend;
}

core::ImageBuffer
RenderDevice::readback() const
{
    if (!p->d.initialized)
        return {};

    const auto& result = p->d.readResult;

    if (result.data.isEmpty())
        return {};

    Q_ASSERT(p->d.targetFormat == TargetFormat::Rgba16F
             && "renderdevice::readback expects the offscreen render target to be RGBA16F.");

    if (p->d.targetFormat != TargetFormat::Rgba16F)
        return {};

    const int w = result.pixelSize.width();
    const int h = result.pixelSize.height();

    if (w <= 0 || h <= 0)
        return {};

    const QRect dataWindow(0, 0, w, h);
    const QRect displayWindow = dataWindow;

    core::ImageBuffer buffer(dataWindow, displayWindow, core::ImageFormat(core::ImageFormat::Half), 4);
    buffer.setPacking(core::ImageBuffer::Packing::Interleaved);
    buffer.setSubsampling(core::ImageBuffer::Subsampling::None);
    buffer.setPixelLayout(core::ImageBuffer::PixelLayout::RGBA);
    buffer.setPixelRange(core::ImageBuffer::PixelRange::Full);
    buffer.allocate();

    const qsizetype dstSize = qsizetype(buffer.byteSize());
    const qsizetype srcSize = result.data.size();

    if (srcSize < dstSize)
        return {};

    std::memcpy(buffer.data(), result.data.constData(), size_t(dstSize));
    return buffer;
}

QSize
RenderDevice::size() const
{
    return p->d.size;
}

core::Error
RenderDevice::error() const
{
    return p->d.error;
}

bool
RenderDevice::isValid() const
{
    return !p->d.error.hasError();
}

}  // namespace flipman::sdk::render
