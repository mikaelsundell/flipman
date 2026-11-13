// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include <flipmansdk/av/media.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/environment.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/plugins/quicktime/quicktimewriter.h>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <cmath>
#include <cstring>
#include <stdexcept>

using namespace flipman::sdk;

namespace {

void
require(bool value, const QString& message)
{
    if (!value)
        throw std::runtime_error(message.toStdString());
}

core::ImageBuffer
frame(core::ImageBuffer::PixelLayout layout)
{
    const QRect bounds(0, 0, 128, 64);
    core::ImageBuffer image(bounds, bounds, core::ImageFormat::UInt8, 4);
    image.setPixelLayout(layout);
    image.setColorSpace(render::ColorSpace::Rec709);
    image.setTransferFunction(render::TransferFunction::Gamma24);
    image.allocate();

    // Test fixture only: a red patch makes an R/B swap unambiguous on decode.
    for (int y = 0; y < 64; ++y) {
        auto* row = image.data() + y * image.strideSize();
        for (int x = 0; x < 128; ++x) {
            const quint8 red = y < 32 ? 240 : 8;
            const quint8 blue = y < 32 ? 8 : 240;
            row[4 * x] = layout == core::ImageBuffer::PixelLayout::RGBA ? red : blue;
            row[4 * x + 1] = 16;
            row[4 * x + 2] = layout == core::ImageBuffer::PixelLayout::RGBA ? blue : red;
            row[4 * x + 3] = 255;
        }
    }
    return image;
}

void
checkMovie(const QString& path, const av::Fps& fps, int count, bool title = true, bool banded = true)
{
    av::Media media;
    require(media.open(core::File(path)) && media.waitForOpened(10000), "could not open encoded movie");
    require(std::abs(media.fps().real() - fps.real()) < 0.001, "movie frame rate differs");
    require(media.timeRange().duration().frames() == count, "movie duration/frame count differs");
    if (title)
        require(media.metaData().value("title").toString() == "Writer round trip", "movie title missing");
    for (int i = 0; i < count; ++i) {
        require(media.read().isValid(), "encoded frame missing");
        const auto image = media.image();
        require(image.dataWindow().size() == QSize(128, 64), "movie dimensions differ");
        require(image.pixelLayout() == core::ImageBuffer::PixelLayout::BGRA, "expected tagged BGRA decode");
        const auto* pixel = image.data() + 16 * image.strideSize() + 16 * 4;
        require(int(pixel[2]) > int(pixel[0]) + 100, "red encoded as blue (channel order differs)");
        if (banded) {
            const auto* bottom = image.data() + 48 * image.strideSize() + 16 * 4;
            require(int(bottom[0]) > int(bottom[2]) + 100, "encoded frame is vertically flipped");
        }
    }
    require(!media.read().isValid(), "encoder added an extra frame");
    media.close();
}

void
roundTrip(const QString& path, const QString& codec, core::ImageBuffer::PixelLayout layout, const av::Fps& fps)
{
    qInfo() << "QuickTime round trip:" << codec << path;
    QScopedPointer<plugins::MediaWriter> writer(
        core::pluginRegistry()->getPlugin<plugins::MediaWriter>(core::File(path).extension()));
    require(writer && dynamic_cast<plugins::QuicktimeWriter*>(writer.data()),
            "registry did not select QuickTime writer");
    require(writer->supportsImage() && !writer->supportsAudio(), "writer capabilities differ");
    writer->setFps(fps);
    writer->setTimeRange(av::TimeRange(av::Time::fromFrames(100, fps), av::Time::fromFrames(3, fps)));
    core::MetaData metadata;
    metadata.insert("title", "Writer round trip");
    require(writer->setMetaData(metadata), "metadata rejected");
    plugins::MediaWriter::Options options;
    options.values.insert("codec", codec);
    require(writer->open(core::File(path), options), writer->error().message());
    for (int i = 0; i < 3; ++i) {
        // Each local handle is released immediately; the encoder must retain storage.
        const auto image = frame(layout);
        const auto stamp = writer->write(image);
        require(stamp.isValid() && stamp.frames() == 100 + i && !writer->error().hasError(), writer->error().message());
        if (i == 0) {
            require(!writer->setMetaData(metadata), "active writer accepted metadata changes");
            writer->setFps(av::Fps::fps30());
            require(writer->fps() == fps && writer->error().hasError(), "active writer changed frame rate");
            require(!writer->write(core::ImageBuffer()).isValid(), "empty image accepted");
        }
    }
    require(writer->time().frames() == 103, "writer cursor did not advance");
    require(!writer->seek(av::TimeRange(av::Time::zero(fps), av::Time::fromFrames(1, fps))).isValid(),
            "active writer accepted seek");
    require(!writer->write(frame(layout)).isValid(), "writer ignored configured end of range");
    require(writer->close(), writer->error().message());
    require(!writer->isOpen() && writer->close(), "close is not idempotent");
    require(QFileInfo(path).size() > 0, "empty movie file");
    checkMovie(path, fps, 3);
    const auto bytes = QFileInfo(path).size();
    require(!writer->open(core::File(path), options) && QFileInfo(path).size() == bytes,
            "existing movie was overwritten");
}

}  // namespace

int
main(int argc, char** argv)
{
    core::Application app(argc, argv);
    try {
        QTemporaryDir directory;
        require(directory.isValid(), "temporary directory unavailable");
        const auto path = [&](const QString& name) { return directory.filePath(name); };
        plugins::QuicktimeWriter writer;
        writer.setFps(av::Fps(0, 1));
        require(writer.error().hasError() && writer.fps() == av::Fps::fps24(), "zero frame rate accepted");
        require(!writer.isOpen() && !writer.write(frame(core::ImageBuffer::PixelLayout::BGRA)).isValid(),
                "closed writer accepted a frame");
        plugins::MediaWriter::Options invalid;
        invalid.values.insert("codec", "invalid");
        require(!writer.open(core::File(path("invalid.mov")), invalid), "invalid codec accepted");
        invalid.values = { { "timeoutMs", -1 } };
        require(!writer.open(core::File(path("invalid.mov")), invalid), "invalid timeout accepted");
        invalid.values = { { "codec", "prores422" } };
        require(!writer.open(core::File(path("invalid.mp4")), invalid), "MP4 accepted ProRes");
        require(writer.open(core::File(path("empty.mov"))), writer.error().message());
        require(!writer.close(), "empty movie reported success");
        roundTrip(path("bgra.mov"), "prores422", core::ImageBuffer::PixelLayout::BGRA, av::Fps(24000, 1001));
        roundTrip(path("rgba.mov"), "prores4444", core::ImageBuffer::PixelLayout::RGBA, av::Fps::fps24());
        roundTrip(path("h264.mp4"), "h264", core::ImageBuffer::PixelLayout::BGRA, av::Fps::fps24());
        roundTrip(path("hevc.mp4"), "hevc", core::ImageBuffer::PixelLayout::BGRA, av::Fps::fps24());

        // Synthetic native NV12 handoff without an RGB conversion in the writer.
        const QRect bounds(0, 0, 128, 64);
        core::ImageBuffer nv12(bounds, bounds, core::ImageFormat::UInt8, 3);
        nv12.setPacking(core::ImageBuffer::Packing::BiPlanar);
        nv12.setSubsampling(core::ImageBuffer::Subsampling::CS420);
        nv12.setPixelLayout(core::ImageBuffer::PixelLayout::NV12);
        nv12.setPixelRange(core::ImageBuffer::PixelRange::Video);
        nv12.setColorSpace(render::ColorSpace::Rec709);
        nv12.setTransferFunction(render::TransferFunction::Gamma24);
        nv12.allocate();
        std::memset(nv12.planeData(0), 63, nv12.planeByteSize(0));
        for (size_t i = 0; i < nv12.planeByteSize(1); i += 2) {
            nv12.planeData(1)[i] = 102;
            nv12.planeData(1)[i + 1] = 240;
        }
        require(writer.open(core::File(path("nv12.mov"))), writer.error().message());
        require(writer.write(nv12).isValid(), writer.error().message());
        require(writer.close(), writer.error().message());
        checkMovie(path("nv12.mov"), av::Fps::fps24(), 1, false, false);
        for (const auto type : { core::ImageFormat::Half, core::ImageFormat::Float }) {
            core::ImageBuffer image(bounds, bounds, type, 4);
            image.setPixelLayout(core::ImageBuffer::PixelLayout::RGBA);
            image.allocate();
            for (int y = 0; y < 64; ++y) {
                for (int x = 0; x < 128; ++x) {
                    auto* pixel = image.data() + y * image.strideSize() + x * image.pixelSize();
                    if (type == core::ImageFormat::Half) {
                        const quint16 values[] = { 0x3b80, 0x2c00, 0x2800, 0x3c00 };
                        std::memcpy(pixel, values, sizeof(values));
                    }
                    else {
                        const float values[] = { 0.9375f, 0.0625f, 0.03125f, 1.0f };
                        std::memcpy(pixel, values, sizeof(values));
                    }
                }
            }
            const auto output = path(type == core::ImageFormat::Half ? "half.mov" : "float.mov");
            plugins::MediaWriter::Options options;
            options.values.insert("codec", "prores4444");
            require(writer.open(core::File(output), options), writer.error().message());
            require(writer.write(image).isValid(), writer.error().message());
            require(writer.close(), writer.error().message());
            checkMovie(output, av::Fps::fps24(), 1, false, false);
        }
        return 0;
    } catch (const std::exception& error) {
        qCritical("%s", error.what());
        return 1;
    }
}
