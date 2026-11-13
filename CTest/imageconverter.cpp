// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "render/private/metalimageconverter.h"

#include <flipmansdk/core/application.h>
#include <flipmansdk/render/imageconverter.h>
#include <QDebug>
#include <rhi/qrhi_platform.h>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>

using namespace flipman::sdk;

namespace {

void
require(bool value, const QString& text)
{
    if (!value)
        throw std::runtime_error(text.toStdString());
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
fixture(core::ImageFormat::Type type, int channels, bool bgra = false)
{
    const QRect bounds(-3, 5, 7, 3);
    core::ImageBuffer raw(bounds, bounds, core::ImageFormat::Float, channels);
    raw.setPixelLayout(channels == 3 ? core::ImageBuffer::PixelLayout::RGB : core::ImageBuffer::PixelLayout::RGBA);
    raw.allocate();
    for (int y = 0; y < 3; ++y) {
        auto* values = reinterpret_cast<float*>(raw.data() + y * raw.strideSize());
        for (int x = 0; x < 7; ++x) {
            values[x * channels] = x == 0 ? -0.25f : (x == 6 ? 2.0f : float(x + 1) / 8);
            values[x * channels + 1] = float(y + 1) / 4;
            values[x * channels + 2] = float(x + y) / 10;
            if (channels == 4)
                values[x * channels + 3] = float((x % 3) + 1) / 3;
        }
    }
    auto result = core::ImageBuffer::convert(raw, type, channels);
    if (bgra)
        result.setPixelLayout(core::ImageBuffer::PixelLayout::BGRA);
    return result;
}

core::ImageBuffer
readback(QRhi* rhi, QRhiCommandBuffer* commands, Target& target, core::ImageFormat::Type type)
{
    QRhiReadbackResult result;
    auto* updates = rhi->nextResourceUpdateBatch();
    updates->readBackTexture({ target.texture.get() }, &result);
    commands->resourceUpdate(updates);
    require(rhi->endOffscreenFrame() == QRhi::FrameOpSuccess && rhi->finish() == QRhi::FrameOpSuccess,
            "GPU completion failed");
    const QRect bounds(QPoint(), target.texture->pixelSize());
    core::ImageBuffer image(bounds, bounds, type, 4);
    image.setPixelLayout(core::ImageBuffer::PixelLayout::RGBA);
    image.allocate();
    require(size_t(result.data.size()) == image.byteSize(), "unexpected readback byte count");
    std::memcpy(image.data(), result.data.constData(), image.byteSize());
    return core::ImageBuffer::convert(image, core::ImageFormat::Float, 4);
}

void
compare(const core::ImageBuffer& reference, const core::ImageBuffer& actual, bool swap, float tolerance)
{
    auto expected = core::ImageBuffer::convert(reference, core::ImageFormat::Float, 4);
    const auto* a = reinterpret_cast<const float*>(actual.data());
    const auto* b = reinterpret_cast<const float*>(expected.data());
    for (int i = 0; i < 7 * 3; ++i)
        for (int c = 0; c < 4; ++c) {
            const int channel = swap && (c == 0 || c == 2) ? 2 - c : c;
            require(std::abs(a[i * 4 + c] - b[i * 4 + channel]) <= tolerance,
                    QString("GPU/CPU difference at pixel %1, channel %2: %3 / %4")
                        .arg(i)
                        .arg(c)
                        .arg(a[i * 4 + c])
                        .arg(b[i * 4 + channel]));
        }
}

}  // namespace

int
main(int argc, char** argv)
{
    core::Application app(argc, argv);
    try {
        QRhiMetalInitParams params;
        std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Metal, &params));
        require(bool(rhi), "native Metal QRhi is unavailable");
        render::ImageConverter converter(rhi.get());
        for (const auto type : { core::ImageFormat::UInt8, core::ImageFormat::Half, core::ImageFormat::Float }) {
            for (int channels : { 3, 4 }) {
                for (const auto destination :
                     { core::ImageFormat::UInt8, core::ImageFormat::Half, core::ImageFormat::Float }) {
                    const auto source = fixture(type, channels);
                    const auto format = destination == core::ImageFormat::UInt8  ? QRhiTexture::RGBA8
                                        : destination == core::ImageFormat::Half ? QRhiTexture::RGBA16F
                                                                                 : QRhiTexture::RGBA32F;
                    Target target(rhi.get(), source.dataWindow().size(), format);
                    QRhiCommandBuffer* commands = nullptr;
                    require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
                    require(converter.convert(commands, source, target.target.get()), converter.error().message());
                    const auto output = readback(rhi.get(), commands, target, destination);
                    compare(core::ImageBuffer::convert(source, destination, 4), output, false,
                            destination == core::ImageFormat::UInt8 ? 1.01f / 255 : 0.001f);
                }
            }
        }
        const auto source = fixture(core::ImageFormat::UInt8, 4, true);
        Target first(rhi.get(), source.dataWindow().size(), QRhiTexture::RGBA8);
        Target second(rhi.get(), source.dataWindow().size(), QRhiTexture::BGRA8);
        render::ImageConverter textureConverter(rhi.get());
        QRhiCommandBuffer* commands = nullptr;
        require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
        require(converter.convert(commands, source, first.target.get()), converter.error().message());
        require(textureConverter.convert(commands, first.texture.get(), second.target.get()),
                textureConverter.error().message());

        // BGRA storage reverses R/B a second time relative to the CPU byte reference.
        compare(source, readback(rhi.get(), commands, second, core::ImageFormat::UInt8), false, 1.01f / 255);
        require(rhi->beginOffscreenFrame(&commands) == QRhi::FrameOpSuccess, "begin frame failed");
        require(!converter.convert(commands, core::ImageBuffer(), first.target.get()), "empty source accepted");
        require(!textureConverter.convert(commands, first.texture.get(), first.target.get()),
                "feedback texture accepted");
        require(rhi->endOffscreenFrame() == QRhi::FrameOpSuccess, "end frame failed");
        textureConverter.releaseResources();

        // Verify actual IOSurface output, including alpha and every row. This
        // is test-only CPU inspection after the encoder bridge completes GPU work.
        render::MetalImageConverter native;
        const auto rgba = fixture(core::ImageFormat::UInt8, 4);
        CVPixelBufferRef buffer = native.convert(rgba);
        require(buffer != nullptr, native.error().message());
        require(CVPixelBufferGetIOSurface(buffer) != nullptr, "encoder output is not IOSurface-backed");
        require(CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly) == kCVReturnSuccess,
                "pixel buffer lock failed");
        const auto* pixels = static_cast<const quint8*>(CVPixelBufferGetBaseAddress(buffer));
        for (int y = 0; y < 3; ++y) {
            const auto* actual = pixels + y * CVPixelBufferGetBytesPerRow(buffer);
            const auto* expected = rgba.data() + y * rgba.strideSize();
            for (int x = 0; x < 7; ++x) {
                require(actual[x * 4] == expected[x * 4 + 2] && actual[x * 4 + 1] == expected[x * 4 + 1]
                            && actual[x * 4 + 2] == expected[x * 4] && actual[x * 4 + 3] == expected[x * 4 + 3],
                        "native BGRA output differs in channel order, row order or alpha");
            }
        }
        CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
        CVPixelBufferRelease(buffer);
        return 0;
    } catch (const std::exception& error) {
        qCritical("%s", error.what());
        return 1;
    }
}
