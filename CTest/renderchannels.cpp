// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include <flipmansdk/av/media.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/render/imagelayer.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderoffscreen.h>
#include <QDebug>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace flipman::sdk;

void
require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int
main(int argc, char** argv)
{
    core::Application app(argc, argv);
    try {
        const QRect bounds(0, 0, 4, 2);
        core::ImageBuffer rgba(bounds, bounds, core::ImageFormat::UInt8, 4);
        rgba.setPixelLayout(core::ImageBuffer::PixelLayout::RGBA);
        rgba.allocate();
        const quint8 pixels[] = { 255, 0,  0,   255, 0,  255, 0,  255, 0, 0, 255, 255, 192, 64,  16,  128,
                                  16,  64, 192, 255, 64, 192, 16, 255, 0, 0, 0,   255, 255, 255, 255, 255 };
        std::copy(std::begin(pixels), std::end(pixels), rgba.data());
        render::RenderOffscreen offscreen;
        require(offscreen.initialize(bounds.size()), "offscreen initialization failed");
        render::RenderEngine engine;
        engine.setResolution(bounds.size());
        engine.setBackground(Qt::transparent);
        offscreen.setRenderEngine(&engine);
        render::ImageLayer layer;
        auto renderImage = [&](const core::ImageBuffer& image) {
            layer.setImage(image);
            engine.setImageLayers({ layer });
            const auto result = offscreen.render();
            require(result.isAllocated(), "render readback failed");
            return core::ImageBuffer::convert(result, core::ImageFormat::Float, 4);
        };
        auto compare = [&](const core::ImageBuffer& expected, const core::ImageBuffer& actual) {
            require(expected.byteSize() == actual.byteSize(), "readback sizes differ");
            const auto* a = reinterpret_cast<const float*>(expected.data());
            const auto* b = reinterpret_cast<const float*>(actual.data());
            for (size_t i = 0; i < actual.byteSize() / sizeof(float); ++i) {
                if (!std::isfinite(b[i]) || std::abs(a[i] - b[i]) >= 0.002f) {
                    for (int p = 0; p < 8; ++p)
                        qCritical() << "pixel" << p << a[4 * p] << a[4 * p + 1] << a[4 * p + 2] << a[4 * p + 3] << "vs"
                                    << b[4 * p] << b[4 * p + 1] << b[4 * p + 2] << b[4 * p + 3];
                    qCritical() << "component" << i << "expected" << a[i] << "actual" << b[i];
                    throw std::runtime_error("BGR/RGB render differs (color or alpha)");
                }
            }
        };
        for (auto type : { core::ImageFormat::UInt8, core::ImageFormat::Half, core::ImageFormat::Float }) {
            auto bgra = rgba;
            bgra.detach();
            for (size_t p = 0; p < bgra.size(); ++p)
                std::swap(bgra.data()[4 * p], bgra.data()[4 * p + 2]);
            bgra.setPixelLayout(core::ImageBuffer::PixelLayout::BGRA);
            const auto normal = core::ImageBuffer::convert(rgba, type, 4);
            const auto reversed = core::ImageBuffer::convert(bgra, type, 4);
            const auto expected = renderImage(normal);
            compare(expected, renderImage(reversed));

            // Reuse the shader/texture cache while switching channel layouts.
            compare(expected, renderImage(normal));
            compare(expected, renderImage(reversed));
            const auto rgb = core::ImageBuffer::convert(rgba, type, 3);
            const auto bgr = core::ImageBuffer::convert(bgra, type, 3);
            const auto expanded = core::ImageBuffer::convert(rgb, 4);
            compare(renderImage(expanded), renderImage(rgb));
            compare(renderImage(expanded), renderImage(bgr));
            qInfo() << "channel ordering passed for pixel type" << int(type);
        }
        const QRect oddBounds(0, 0, 7, 3);

        // initialize() replaces the QRhi; release cached engine resources first.
        engine.reset();
        engine.setResolution(oddBounds.size());
        require(offscreen.initialize(oddBounds.size()), "odd-size offscreen initialization failed");
        for (const auto type : { core::ImageFormat::UInt8, core::ImageFormat::Half, core::ImageFormat::Float }) {
            for (int channels : { 1, 2, 3, 4, 3, 2, 1 }) {
                core::ImageBuffer packed(oddBounds, oddBounds, core::ImageFormat::Float, channels);
                packed.setPixelLayout(channels == 4 ? core::ImageBuffer::PixelLayout::RGBA
                                                    : core::ImageBuffer::PixelLayout::RGB);
                packed.allocate();
                auto* samples = reinterpret_cast<float*>(packed.data());
                for (size_t pixel = 0; pixel < packed.size(); ++pixel)
                    for (int c = 0; c < channels; ++c)
                        samples[pixel * channels + c] = float((pixel + c) % 7) / 8;
                const auto native = core::ImageBuffer::convert(packed, type, channels);
                compare(renderImage(core::ImageBuffer::convert(native, type, 4)), renderImage(native));
            }
        }
        engine.reset();
        engine.setResolution(bounds.size());
        require(offscreen.initialize(bounds.size()), "restore offscreen initialization failed");
        const auto packedRgb = core::ImageBuffer::convert(rgba, core::ImageFormat::UInt8, 3);
        core::ImageBuffer nv12(bounds, bounds, core::ImageFormat::UInt8, 1);
        nv12.setPixelLayout(core::ImageBuffer::PixelLayout::NV12);
        nv12.setPacking(core::ImageBuffer::Packing::BiPlanar);
        nv12.setSubsampling(core::ImageBuffer::Subsampling::CS420);
        nv12.allocate();
        std::fill(nv12.data(), nv12.data() + nv12.byteSize(), quint8(128));
        const auto expectedNv12 = renderImage(nv12);
        renderImage(packedRgb);
        compare(expectedNv12, renderImage(nv12));
        core::ImageBuffer uyvy(bounds, bounds, core::ImageFormat::UInt8, 2);
        uyvy.setPixelLayout(core::ImageBuffer::PixelLayout::UYVY);
        uyvy.setPacking(core::ImageBuffer::Packing::Packed);
        uyvy.setSubsampling(core::ImageBuffer::Subsampling::CS422);
        uyvy.allocate();
        std::fill(uyvy.data(), uyvy.data() + uyvy.byteSize(), quint8(128));
        const auto expectedUyvy = renderImage(uyvy);
        renderImage(packedRgb);
        compare(expectedUyvy, renderImage(uyvy));
        compare(renderImage(core::ImageBuffer::convert(packedRgb, 4)), renderImage(packedRgb));
        if (argc > 1) {
            const QString path = QString::fromUtf8(argv[1]);
            if (!QFileInfo::exists(path)) {
                qWarning() << "QuickTime fixture missing:" << path;
                return qEnvironmentVariableIsSet("FLIPMAN_REQUIRE_CHANNEL_DATA") ? 1 : 77;
            }
            av::Media media;
            require(media.open(core::File(path)) && media.waitForOpened(5000), "QuickTime open failed");
            require(media.read().isValid(), "QuickTime frame read failed");
            const auto decoded = media.image();
            require(decoded.isAllocated() && decoded.pixelLayout() == core::ImageBuffer::PixelLayout::BGRA
                        && decoded.imageFormat().type() == core::ImageFormat::UInt8,
                    "QuickTime must identify BGRA pixels");
            auto reference = core::ImageBuffer::convert(decoded, core::ImageFormat::UInt8, 4);
            bool colored = false;
            for (size_t p = 0; p < reference.size(); ++p) {
                auto* pixel = reference.data() + 4 * p;
                colored = colored || std::abs(int(pixel[0]) - int(pixel[2])) > 16;
                std::swap(pixel[0], pixel[2]);
            }
            require(colored, "QuickTime fixture must contain unequal red/blue channels");
            reference.setPixelLayout(core::ImageBuffer::PixelLayout::RGBA);
            compare(renderImage(reference), renderImage(decoded));
            qInfo() << "QuickTime BGRA frame matches explicit RGBA reference";
        }
        return 0;
    } catch (const std::exception& error) {
        qCritical() << error.what();
        return 1;
    }
}
