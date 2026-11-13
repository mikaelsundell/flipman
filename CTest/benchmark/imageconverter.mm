// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "render/private/metalimageconverter.h"

#include <flipmansdk/core/application.h>
#include <flipmansdk/render/imageconverter.h>
#include <flipmansdk/render/imagelayer.h>
#include <flipmansdk/render/rendercontext.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderspec.h>
#include <CoreImage/CoreImage.h>
#include <Metal/Metal.h>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <rhi/qrhi_platform.h>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <stdexcept>
#if defined(__ARM_NEON)
#    include <arm_neon.h>
#endif

using namespace flipman::sdk;

namespace {

void
require(bool value, const QString& text)
{
    if (!value)
        throw std::runtime_error(text.toStdString());
}

double
cpuTime()
{
    timespec time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
    return double(time.tv_sec) * 1000 + double(time.tv_nsec) / 1000000;
}

struct Target {
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;
    std::unique_ptr<QRhiTextureRenderTarget> target;

    Target(QRhi* rhi, const QSize& size, QRhiTexture::Format format)
    {
        texture.reset(rhi->newTexture(format, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        require(texture->create(), "texture creation failed");
        target.reset(rhi->newTextureRenderTarget({ texture.get() }));
        pass.reset(target->newCompatibleRenderPassDescriptor());
        target->setRenderPassDescriptor(pass.get());
        require(target->create(), "target creation failed");
    }
};

core::ImageBuffer
input(const QSize& size, core::ImageFormat::Type type, int channels)
{
    const QRect bounds(QPoint(), size);
    core::ImageBuffer raw(bounds, bounds, core::ImageFormat::Float, channels);
    raw.setPixelLayout(channels == 3 ? core::ImageBuffer::PixelLayout::RGB : core::ImageBuffer::PixelLayout::RGBA);
    raw.allocate();
    for (int y = 0; y < size.height(); ++y) {
        auto* row = reinterpret_cast<float*>(raw.data() + y * raw.strideSize());
        for (int x = 0; x < size.width(); ++x) {
            row[x * channels] = float(x % 251) / 255;
            row[x * channels + 1] = 0.5f;
            row[x * channels + 2] = float(y % 251) / 255;
            if (channels == 4)
                row[x * channels + 3] = 1;
        }
    }
    return core::ImageBuffer::convert(raw, type, channels);
}

// Benchmark-only swizzle reference: ImageBuffer::convert does not change RGBA
// versus BGRA layout. The SDK's existing NEON 3->4 paths are measured separately.
core::ImageBuffer
cpuBgra(const core::ImageBuffer& source, bool neon)
{
    core::ImageBuffer output(source.dataWindow(), source.displayWindow(), core::ImageFormat::UInt8, 4);
    output.setPixelLayout(core::ImageBuffer::PixelLayout::BGRA);
    output.allocate();
    for (int y = 0; y < source.dataWindow().height(); ++y) {
        const auto* src = source.data() + y * source.strideSize();
        auto* dst = output.data() + y * output.strideSize();
        int x = 0;
#if defined(__ARM_NEON)
        if (neon) {
            for (; x + 16 <= source.dataWindow().width(); x += 16) {
                auto v = vld4q_u8(src + x * 4);
                std::swap(v.val[0], v.val[2]);
                vst4q_u8(dst + x * 4, v);
            }
        }
#else
        Q_UNUSED(neon);
#endif
        for (; x < source.dataWindow().width(); ++x) {
            dst[x * 4] = src[x * 4 + 2];
            dst[x * 4 + 1] = src[x * 4 + 1];
            dst[x * 4 + 2] = src[x * 4];
            dst[x * 4 + 3] = src[x * 4 + 3];
        }
    }
    return output;
}

struct Sample {
    double wall;
    double cpu;
    double record;
};

volatile quint8 sink = 0;

template<typename F>
QJsonObject
measure(int iterations, F&& run)
{
    std::vector<Sample> samples;
    for (int i = -5; i < iterations; ++i) {
        double record = 0;
        QElapsedTimer wall;
        wall.start();
        const double cpu = cpuTime();
        run(record);
        Sample sample { double(wall.nsecsElapsed()) / 1000000, cpuTime() - cpu, record };
        if (i >= 0)
            samples.push_back(sample);
    }
    auto quantile = [&](auto member, double fraction) {
        std::vector<double> values;
        for (const auto& sample : samples)
            values.push_back(sample.*member);
        std::sort(values.begin(), values.end());
        return values[size_t(fraction * (values.size() - 1))];
    };
    return { { "median_ms", quantile(&Sample::wall, 0.5) },
             { "p95_ms", quantile(&Sample::wall, 0.95) },
             { "thread_cpu_ms", quantile(&Sample::cpu, 0.5) },
             { "record_ms", quantile(&Sample::record, 0.5) } };
}

// Previous writer implementation, retained only as a performance baseline.
struct LegacyCoreImage {
    CIContext* context = nil;
    CVPixelBufferPoolRef pool = nullptr;

    explicit LegacyCoreImage(const QSize& size)
    {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        context = [[CIContext contextWithMTLDevice:device
                                           options:@{
                                               kCIContextUseSoftwareRenderer : @NO,
                                               kCIContextWorkingColorSpace : [NSNull null],
                                               kCIContextOutputColorSpace : [NSNull null]
                                           }] retain];
        [device release];
        NSDictionary* attrs = @{
            (NSString*)kCVPixelBufferWidthKey : @(size.width()),
            (NSString*)kCVPixelBufferHeightKey : @(size.height()),
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
            (NSString*)kCVPixelBufferMetalCompatibilityKey : @YES,
            (NSString*)kCVPixelBufferIOSurfacePropertiesKey : @ {}
        };
        require(context && CVPixelBufferPoolCreate(nullptr, nullptr, (CFDictionaryRef)attrs, &pool) == kCVReturnSuccess,
                "Core Image baseline unavailable");
    }

    ~LegacyCoreImage()
    {
        if (pool)
            CVPixelBufferPoolRelease(pool);
        [context release];
    }

    void run(const core::ImageBuffer& image)
    {
        @autoreleasepool {
            CVPixelBufferRef output = nullptr;
            require(CVPixelBufferPoolCreatePixelBuffer(nullptr, pool, &output) == kCVReturnSuccess,
                    "CI output allocation failed");
            NSData* bytes = [NSData dataWithBytesNoCopy:image.data() length:image.byteSize() freeWhenDone:NO];
            CIImage* source = [CIImage
                imageWithBitmapData:bytes
                        bytesPerRow:image.strideSize()
                               size:CGSizeMake(image.dataWindow().width(), image.dataWindow().height())
                             format:kCIFormatRGBA8
                         colorSpace:nil];
            [context render:source
                toCVPixelBuffer:output
                         bounds:CGRectMake(0, 0, image.dataWindow().width(), image.dataWindow().height())
                     colorSpace:nil];

            // Include visibility/completion, not just dispatch to the Metal queue.
            require(CVPixelBufferLockBaseAddress(output, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess,
                    "CI output lock failed");
            sink = static_cast<const quint8*>(CVPixelBufferGetBaseAddress(output))[0];
            CVPixelBufferUnlockBaseAddress(output, kCVPixelBufferLock_ReadOnly);
            CVPixelBufferRelease(output);
        }
    }
};

}  // namespace

int
main(int argc, char** argv)
{
    core::Application app(argc, argv);
    try {
#if defined(QT_DEBUG)
        throw std::runtime_error("run the benchmark in Release with Release dependencies");
#endif
        const int iterations = argc > 1 ? QString::fromUtf8(argv[1]).toInt() : 20;
        require(iterations >= 3 && iterations <= 1000, "iterations must be between 3 and 1000");
        QRhiMetalInitParams params;
        std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Metal, &params));
        require(bool(rhi), "Metal QRhi unavailable");
        const auto* handles = static_cast<const QRhiMetalNativeHandles*>(rhi->nativeHandles());
        id<MTLDevice> device = (id<MTLDevice>)handles->dev;
        QJsonObject report { { "qt", QString::fromLatin1(qVersion()) },
                             { "os", QSysInfo::prettyProductName() },
                             { "gpu", QString::fromUtf8(device.name.UTF8String) },
                             { "iterations", iterations },
                             { "warmup", 5 },
                             { "configuration", QStringLiteral("Release") } };
        QJsonArray rows;
        for (const QSize size : { QSize(1920, 1080), QSize(3840, 2160) }) {
            auto add = [&](const QString& operation, const QString& path, QJsonObject metrics) {
                metrics.insert("size", QString("%1x%2").arg(size.width()).arg(size.height()));
                metrics.insert("operation", operation);
                metrics.insert("path", path);
                rows.append(metrics);
            };
            for (const auto type : { core::ImageFormat::UInt8, core::ImageFormat::Half, core::ImageFormat::Float }) {
                const auto source = input(size, type, 3);
                const auto fmt = type == core::ImageFormat::UInt8  ? QRhiTexture::RGBA8
                                 : type == core::ImageFormat::Half ? QRhiTexture::RGBA16F
                                                                   : QRhiTexture::RGBA32F;
                const QString operation = type == core::ImageFormat::UInt8  ? "rgb8_to_rgba8"
                                          : type == core::ImageFormat::Half ? "rgb_half_to_rgba_half"
                                                                            : "rgb_float_to_rgba_float";
                add(operation, "cpu_sdk_neon", measure(iterations, [&](double&) {
                        auto out = render::ImageConverter::convertCpu(source, 4);
                        sink = out.data()[0];
                    }));
                Target target(rhi.get(), size, fmt);
                render::ImageConverter converter(rhi.get());
                for (bool download : { false, true }) {
                    add(operation, download ? "gpu_upload_convert_readback" : "gpu_upload_convert_resident_output",
                        measure(iterations, [&](double& record) {
                            QRhiCommandBuffer* commands = nullptr;
                            QElapsedTimer clock;
                            clock.start();
                            require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
                            if (!converter.record(commands, source, target.target.get()))
                                throw std::runtime_error(converter.error().message().toStdString());
                            record = double(clock.nsecsElapsed()) / 1000000;
                            QRhiReadbackResult result;
                            if (download) {
                                auto* updates = rhi->nextResourceUpdateBatch();
                                updates->readBackTexture({ target.texture.get() }, &result);
                                commands->resourceUpdate(updates);
                            }
                            require(rhi->endOffscreenFrame() == QRhi::FrameOpSuccess
                                        && rhi->finish() == QRhi::FrameOpSuccess,
                                    "GPU completion failed");
                            if (download) {
                                require(!result.data.isEmpty(), "empty readback");
                                sink = quint8(result.data[0]);
                            }
                        }));
                }

                // Compare the actual fused layer path against both the previous
                // general expansion and the fastest existing CPU channel kernel.
                // Alternate preallocated inputs so every frame uploads fresh data.
                auto alternate = source;
                alternate.detach();
                std::memcpy(alternate.data(), source.data(), source.byteSize());
                for (int mode : { 0, 1, 2 }) {
                    Target layerTarget(rhi.get(), size, QRhiTexture::RGBA16F);
                    render::RenderSurface surface;
                    surface.setRenderTarget(layerTarget.target.get());
                    render::RenderContext context;
                    context.setRhi(rhi.get());
                    context.setSurface(surface);
                    render::RenderSpec spec;
                    spec.setSize(size);
                    render::RenderEngine engine;
                    engine.setResolution(size);
                    render::ImageLayer layer;
                    int frame = 0;
                    const QString path = mode == 0   ? "previous_cpu_general_then_layer"
                                         : mode == 1 ? "cpu_neon_then_layer"
                                                     : "gpu_fused_native_layer";
                    add("layer_" + operation, path, measure(iterations, [&](double& record) {
                            QElapsedTimer clock;
                            clock.start();
                            const auto& next = (++frame % 2) ? source : alternate;
                            auto prepared = mode == 0   ? render::ImageConverter::convertCpu(next, type, 4)
                                            : mode == 1 ? render::ImageConverter::convertCpu(next, 4)
                                                        : next;
                            layer.setImage(prepared);
                            engine.setImageLayers({ layer });
                            QRhiCommandBuffer* commands = nullptr;
                            require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
                            require(engine.initialize(context, spec), "layer initialization failed");
                            engine.render(context, spec, commands);
                            record = double(clock.nsecsElapsed()) / 1000000;
                            require(rhi->endOffscreenFrame() == QRhi::FrameOpSuccess
                                        && rhi->finish() == QRhi::FrameOpSuccess,
                                    "layer completion failed");
                            if (engine.error().hasError())
                                throw std::runtime_error(engine.error().message().toStdString());
                        }));
                }
            }
            {
                const auto source = input(size, core::ImageFormat::Float, 4);
                add("rgba_float_to_rgba_half", "cpu_sdk_scalar", measure(iterations, [&](double&) {
                        auto out = render::ImageConverter::convertCpu(source, core::ImageFormat::Half, 4);
                        sink = out.data()[0];
                    }));
                Target gpuSource(rhi.get(), size, QRhiTexture::RGBA32F);
                Target output(rhi.get(), size, QRhiTexture::RGBA16F);
                render::ImageConverter uploader(rhi.get()), converter(rhi.get());
                QRhiCommandBuffer* commands = nullptr;
                require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
                require(uploader.record(commands, source, gpuSource.target.get()), "seed upload failed");
                rhi->endOffscreenFrame();
                rhi->finish();
                add("rgba_float_to_rgba_half", "gpu_resident_texture", measure(iterations, [&](double& record) {
                        QElapsedTimer clock;
                        clock.start();
                        require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
                        require(converter.record(commands, gpuSource.texture.get(), output.target.get()),
                                "resident conversion failed");
                        record = double(clock.nsecsElapsed()) / 1000000;
                        rhi->endOffscreenFrame();
                        rhi->finish();
                    }));
                converter.releaseResources();
                render::ImageConverter ingress(rhi.get());
                for (bool download : { false, true }) {
                    add("rgba_float_to_rgba_half",
                        download ? "gpu_upload_convert_readback" : "gpu_upload_convert_resident_output",
                        measure(iterations, [&](double& record) {
                            QElapsedTimer clock;
                            clock.start();
                            require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
                            if (!ingress.record(commands, source, output.target.get()))
                                throw std::runtime_error(ingress.error().message().toStdString());
                            record = double(clock.nsecsElapsed()) / 1000000;
                            QRhiReadbackResult result;
                            if (download) {
                                auto* updates = rhi->nextResourceUpdateBatch();
                                updates->readBackTexture({ output.texture.get() }, &result);
                                commands->resourceUpdate(updates);
                            }
                            require(rhi->endOffscreenFrame() == QRhi::FrameOpSuccess
                                        && rhi->finish() == QRhi::FrameOpSuccess,
                                    "GPU completion failed");
                            if (download) {
                                require(!result.data.isEmpty(), "empty readback");
                                sink = quint8(result.data[0]);
                            }
                        }));
                }
            }
            {
                const auto source = input(size, core::ImageFormat::UInt8, 4);
                for (bool neon : { false, true })
                    add("rgba8_to_bgra8", neon ? "cpu_neon_reference" : "cpu_scalar_reference",
                        measure(iterations, [&](double&) {
                            auto out = cpuBgra(source, neon);
                            sink = out.data()[0];
                        }));
                render::MetalImageConverter encoder;
                add("rgba8_to_bgra8", "qrhi_metal_encoder_surface", measure(iterations, [&](double&) {
                        CVPixelBufferRef output = encoder.convert(source);
                        if (!output)
                            throw std::runtime_error(encoder.error().message().toStdString());
                        CVPixelBufferRelease(output);
                    }));
                LegacyCoreImage baseline(size);
                add("rgba8_to_bgra8", "previous_coreimage_completed",
                    measure(iterations, [&](double&) { baseline.run(source); }));
            }
        }
        report.insert("results", rows);
        std::cout << QJsonDocument(report).toJson(QJsonDocument::Indented).constData();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
