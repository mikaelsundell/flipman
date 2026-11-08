// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
#include <flipmansdk/plugins/braw/brawreader.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <memory>
#include <stdexcept>

using namespace flipman::sdk;
void
require(bool condition, const QString& message)
{
    if (!condition)
        throw std::runtime_error(message.toStdString());
}
qint64
frame(const av::Time& time)
{
    require(time.isValid(), "expected a valid frame time");
    return time.frames();
}
QByteArray
digest(const core::ImageBuffer& image)
{
    return QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char*>(image.data()), image.byteSize()),
                                    QCryptographicHash::Sha256);
}
int
main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        plugins::BrawReader reader;
        require(!reader.isOpen() && !reader.time().isValid(), "new reader has a time/open state");
        require(!reader.read().isValid() && reader.error().hasError(), "closed read should fail");
        require(reader.close() && !reader.error().hasError(), "close should clear state");
        require(reader.supportsImage() && !reader.supportsAudio() && reader.supportsConcurrent(),
                "incorrect capabilities");
        require(!reader.open(core::File("/nonexistent/clip.braw")) && !reader.isOpen() && reader.error().hasError(),
                "missing file accepted");
        QTemporaryDir dir;
        require(dir.isValid(), "temp directory failed");
        QFile invalid(dir.filePath("invalid.braw"));
        require(invalid.open(QIODevice::WriteOnly), "fixture open failed");
        invalid.write("not a Blackmagic RAW clip");
        invalid.close();
        require(!reader.open(core::File(invalid.fileName())) && !reader.isOpen() && reader.error().hasError(),
                "invalid clip accepted");
        plugins::PluginRegistry registry;
#ifdef FLIPMAN_TEST_BRAW_ENABLED
        std::unique_ptr<core::Plugin> plugin(registry.getPlugin<plugins::MediaReader>("BRAW"));
        require(dynamic_cast<plugins::BrawReader*>(plugin.get()), "BRAW reader not registered");
#else
        require(!registry.hasExtension<plugins::MediaReader>("braw"), "disabled reader registered");
#endif
        if (argc < 2 || QString::fromUtf8(argv[1]) == "state")
            return 0;
        const QString path = argc > 2 ? QString::fromUtf8(argv[2]) : QString();
        if (!QFile::exists(path)) {
            qWarning() << "BRAW fixture missing; configure FLIPMAN_BRAW_TEST_FILE";
            return qEnvironmentVariableIsSet("FLIPMAN_REQUIRE_BRAW_DATA") ? 1 : 77;
        }
        require(reader.open(core::File(path)), reader.error().message());
        require(reader.isOpen() && reader.fps().isValid() && reader.timeRange().isValid(), "invalid opened properties");
        const auto readFrame = [&] {
            const auto time = reader.read();
            require(time.isValid(),
                    reader.error().message().isEmpty() ? "invalid decoded timestamp" : reader.error().message());
            return time.frames();
        };
        const auto fps = reader.fps();
        const auto total = reader.timeRange().duration().frames();
        require(total > 2, "fixture needs at least three frames");
        qInfo() << "BRAW clip" << total << "frames at" << fps.real();
        require(readFrame() == 0 && !reader.error().hasError(), reader.error().message());
        const auto first = reader.image();
        require(first.isAllocated() && first.channels() == 3 && first.imageFormat().type() == core::ImageFormat::Float,
                "expected owned RGB float32 image");
        const QByteArray firstHash = digest(first);
        const auto* pixels = reinterpret_cast<const float*>(first.data());
        for (size_t i = 0; i < first.byteSize() / sizeof(float); i += 997)
            require(std::isfinite(pixels[i]), "nonfinite pixels");
        require(!reader.metaData().keys(core::MetaData::Group::Camera).isEmpty(), "camera metadata missing");
        require(readFrame() == 1 && !reader.error().hasError(), reader.error().message());
        require(digest(first) == firstHash, "previous frame storage was overwritten");
        const auto seek = [&](qint64 start, qint64 duration) {
            return reader.seek(av::TimeRange(av::Time::fromFrames(start, fps), av::Time::fromFrames(duration, fps)));
        };
        require(frame(seek(total / 2, 1)) == total / 2, "middle seek failed");
        require(readFrame() == total / 2, "middle frame read failed");
        require(!reader.read().isValid() && !reader.error().hasError() && !reader.image().isAllocated(),
                "range EOF is not clean");
        require(frame(seek(total - 1, 1)) == total - 1, "last seek failed");
        require(frame(reader.skip()) == total - 1 && !reader.read().isValid(), "skip/last EOF failed");
        require(frame(seek(total - 1, 1)) == total - 1 && readFrame() == total - 1, "last decode failed");
        require(!seek(total, 1).isValid() && reader.error().hasError(), "out-of-range seek accepted");
        require(frame(seek(0, total)) == 0 && !reader.error().hasError(), "seek did not recover error");
        require(readFrame() == 0 && digest(reader.image()) == firstHash, "seek-back pixels differ");
        plugins::BrawReader second;
        plugins::MediaReader::Options options;
        options.values.insert("exposure", 1.0);
        options.values.insert("tint", -10);
        require(second.open(core::File(path), options), second.error().message());
        require(second.read().isValid() && !second.error().hasError(), second.error().message());
        require(digest(second.image()) != firstHash, "processing overrides did not change pixels");
        require(second.close() && reader.close() && !reader.isOpen() && !reader.image().isAllocated(),
                "close did not clear resources");
        require(reader.open(core::File(path)), reader.error().message());
        require(readFrame() == 0, "reopen failed");
        options.values.insert("tint", 9999);
        require(!second.open(core::File(path), options) && second.error().hasError(), "invalid options accepted");
        return 0;
    } catch (const std::exception& error) {
        qCritical() << error.what();
        return 1;
    }
}
