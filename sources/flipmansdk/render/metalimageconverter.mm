// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "private/metalimageconverter.h"

#include <flipmansdk/render/imageconverter.h>
#include <Metal/Metal.h>
#include <QThread>
#include <rhi/qrhi_platform.h>
#include <memory>

namespace flipman::sdk::render {

class MetalImageConverterPrivate {
public:
    ~MetalImageConverterPrivate();
    bool prepare(const QSize& size);
    bool fail(const QString& text);

    struct Data {
        CVPixelBufferPoolRef pool = nullptr;
        core::Error error;
        QThread* thread = nullptr;
        QSize size;
        std::unique_ptr<QRhi> rhi;
        std::unique_ptr<ImageConverter> converter;
    };
    Data d;
};

MetalImageConverterPrivate::~MetalImageConverterPrivate()
{
    if (d.rhi)
        d.rhi->finish();
    if (d.pool)
        CVPixelBufferPoolRelease(d.pool);
}

bool
MetalImageConverterPrivate::fail(const QString& text)
{
    d.error = core::Error("metalimageconverter", text);
    return false;
}

bool
MetalImageConverterPrivate::prepare(const QSize& size)
{
    if (d.thread && d.thread != QThread::currentThread())
        return fail("use the converter on its creating thread");

    if (size.isEmpty())
        return fail("invalid image dimensions");

    if (!d.rhi) {
        QRhiMetalInitParams params;
        d.rhi.reset(QRhi::create(QRhi::Metal, &params));
        if (!d.rhi)
            return fail("Qt's native Metal device is unavailable");

        d.thread = QThread::currentThread();
        d.converter = std::make_unique<ImageConverter>(d.rhi.get());
    }
    if (!d.pool || d.size != size) {
        if (d.pool)
            CVPixelBufferPoolRelease(d.pool);
        d.pool = nullptr;
        NSDictionary* attributes = @{
            (NSString*)kCVPixelBufferWidthKey : @(size.width()),
            (NSString*)kCVPixelBufferHeightKey : @(size.height()),
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
            (NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
            (NSString*)kCVPixelBufferIOSurfacePropertiesKey : @ {}
        };
        if (CVPixelBufferPoolCreate(kCFAllocatorDefault, nullptr, (CFDictionaryRef)attributes, &d.pool)
            != kCVReturnSuccess)
            return fail("could not create encoder pixel-buffer pool");

        d.size = size;
    }
    d.error = {};
    return true;
}

MetalImageConverter::MetalImageConverter()
    : p(new MetalImageConverterPrivate)
{}

MetalImageConverter::~MetalImageConverter() = default;

CVPixelBufferRef
MetalImageConverter::convert(const core::ImageBuffer& image)
{
    @autoreleasepool {
        if (!p->prepare(image.dataWindow().size()))
            return nullptr;

        auto& d = p->d;
        CVPixelBufferRef buffer = nullptr;
        if (CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, d.pool, &buffer) != kCVReturnSuccess) {
            p->fail("could not allocate encoder output buffer");
            return nullptr;
        }
        const auto* handles = static_cast<const QRhiMetalNativeHandles*>(d.rhi->nativeHandles());
        id<MTLDevice> device = (id<MTLDevice>)handles->dev;
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                         width:d.size.width()
                                        height:d.size.height()
                                     mipmapped:NO];
        descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        descriptor.storageMode = MTLStorageModeShared;
        id<MTLTexture> native =
            [device newTextureWithDescriptor:descriptor iosurface:CVPixelBufferGetIOSurface(buffer) plane:0];
        bool ok = false;
        if (native) {
            {
                // No output upload or readback: QRhi renders into the encoder's IOSurface.
                std::unique_ptr<QRhiTexture> texture(
                    d.rhi->newTexture(QRhiTexture::BGRA8, d.size, 1, QRhiTexture::RenderTarget));
                if (texture->createFrom({ quint64((quintptr)native), 0 })) {
                    std::unique_ptr<QRhiRenderPassDescriptor> pass;
                    std::unique_ptr<QRhiTextureRenderTarget> target(d.rhi->newTextureRenderTarget({ texture.get() }));
                    pass.reset(target->newCompatibleRenderPassDescriptor());
                    target->setRenderPassDescriptor(pass.get());
                    QRhiCommandBuffer* commands = nullptr;
                    if (target->create() && d.rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess) {
                        ok = d.converter->record(commands, image, target.get());
                        const auto submitted = d.rhi->endOffscreenFrame();

                        // AVFoundation can consume the surface after this return. Wait
                        // for completed work here, never inside the recording API.
                        const auto completed = d.rhi->finish();
                        ok = ok && submitted == QRhi::FrameOpSuccess && completed == QRhi::FrameOpSuccess;
                    }
                }
            }
            [native release];
        }
        if (!ok) {
            CVPixelBufferRelease(buffer);
            d.error = d.converter->error();
            if (!d.error.hasError())
                p->fail("native Metal texture import or conversion failed");
            return nullptr;
        }
        return buffer;
    }
}

core::Error
MetalImageConverter::error() const
{
    return p->d.error;
}

}  // namespace flipman::sdk::render
