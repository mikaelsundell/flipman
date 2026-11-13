// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/render/imageconverter.h>
#include <QCoreApplication>
#include <QDebug>
#include <cmath>
#include <cstring>
#include <stdexcept>

using namespace flipman::sdk;

namespace {

void
require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void
checkPixels(const core::ImageBuffer& image, int channels)
{
    const auto decoded = render::ImageConverter::convertCpu(image, core::ImageFormat::Float, channels);
    const auto* values = reinterpret_cast<const float*>(decoded.data());
    for (size_t pixel = 0; pixel < decoded.size(); ++pixel)
        for (int c = 0; c < channels; ++c) {
            const float expected = c < 3 ? float(c + 1) / 4 : 1.0f;
            require(std::isfinite(values[pixel * channels + c])
                        && std::abs(values[pixel * channels + c] - expected) <= 1.01f / 127,
                    "converted CPU sample differs from expected normalized value");
        }
}

}  // namespace

int
main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        const QRect data(-3, 5, 17, 3);
        const QRect display(-5, 3, 21, 7);
        core::ImageBuffer source(data, display, core::ImageFormat::Float, 3);
        source.setPixelLayout(core::ImageBuffer::PixelLayout::BGR);
        source.setPixelRange(core::ImageBuffer::PixelRange::Full);
        source.setColorSpace(render::ColorSpace::Rec2020);
        source.setTransferFunction(render::TransferFunction::Linear);
        source.allocate();
        auto* samples = reinterpret_cast<float*>(source.data());
        for (size_t pixel = 0; pixel < source.size(); ++pixel)
            for (int c = 0; c < 3; ++c)
                samples[pixel * 3 + c] = float(c + 1) / 4;

        // No GUI application or rendering device is needed for CPU conversion.
        render::ImageConverter converter;
        require(!converter.record(nullptr, source, nullptr) && converter.error().hasError(),
                "CPU-only facade accepted GPU recording without a device");
        for (const auto type :
             { core::ImageFormat::UInt8, core::ImageFormat::Int8, core::ImageFormat::UInt16, core::ImageFormat::Int16,
               core::ImageFormat::UInt32, core::ImageFormat::Int32, core::ImageFormat::UInt64, core::ImageFormat::Int64,
               core::ImageFormat::Half, core::ImageFormat::Float, core::ImageFormat::Double }) {
            const auto converted = render::ImageConverter::convertCpu(source, type, 4);
            require(converted.isAllocated() && converted.dataWindow() == data && converted.displayWindow() == display,
                    "CPU conversion lost geometry");
            require(converted.pixelLayout() == core::ImageBuffer::PixelLayout::BGRA
                        && converted.pixelRange() == source.pixelRange()
                        && converted.colorSpace() == source.colorSpace()
                        && converted.transferFunction() == source.transferFunction(),
                    "CPU conversion lost source tags");
            checkPixels(converted, 4);
            const auto legacy = core::ImageBuffer::convert(source, type, 4);
            require(legacy.byteSize() == converted.byteSize()
                        && std::memcmp(legacy.data(), converted.data(), converted.byteSize()) == 0,
                    "ImageBuffer convenience wrapper differs from facade");
        }
        for (const auto type : { core::ImageFormat::UInt8, core::ImageFormat::Half, core::ImageFormat::Float }) {
            const auto rgb = render::ImageConverter::convertCpu(source, type, 3);
            const auto rgba = render::ImageConverter::convertCpu(rgb, 4);
            checkPixels(rgba, 4);
            const auto reduced = render::ImageConverter::convertCpu(rgba, 2);
            checkPixels(reduced, 2);
            const auto shared = render::ImageConverter::convertCpu(rgb, 3);
            const auto detached = render::ImageConverter::convertCpu(rgb, type, 3);
            require(shared.data() == rgb.data(), "channel-only identity should share storage");
            require(detached.data() != rgb.data(), "format-and-channel identity should detach storage");
            require(std::memcmp(detached.data(), rgb.data(), rgb.byteSize()) == 0,
                    "identity conversion changed pixels");
        }
        return 0;
    } catch (const std::exception& error) {
        qCritical() << error.what();
        return 1;
    }
}
