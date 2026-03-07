// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "testsdk.h"

#include <flipmansdk/av/clip.h>
#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/media.h>
#include <flipmansdk/av/mediaprocessor.h>
#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timeline.h>
#include <flipmansdk/av/timer.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/av/track.h>

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/dispatchgroup.h>
#include <flipmansdk/core/environment.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/filerange.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/log.h>
#include <flipmansdk/core/system.h>

#include <flipmansdk/plugins/effectreader.h>
#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/mediawriter.h>

#include <flipmansdk/plugins/pluginregistry.h>

#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderoffscreen.h>
#include <flipmansdk/render/shadercompiler.h>
#include <flipmansdk/render/shadercomposer.h>
#include <flipmansdk/render/shaderdefinition.h>

#include <QApplication>
#include <QDebug>
#include <QThread>
#include <rhi/qrhi.h>

#include <iostream>

namespace flipman::sdk::test {
namespace {

    QString dataPath;
    QString testPath;
    template<typename T> bool testValue(const T& value, const T& expected, const char* label = nullptr)
    {
        if (value != expected) {
            if (label)
                core::logErr() << label << "mismatch:"
                               << "expected:" << expected << "got:" << value << Qt::endl;
            else
                core::logErr() << "value mismatch:"
                               << "expected:" << expected << "got:" << value << Qt::endl;
            return false;
        }
        return true;
    }

    bool testMetaData(const core::MetaData& meta, const QString& key, const QString& expected)
    {
        if (!meta.isValid()) {
            core::logErr() << "metadata invalid" << Qt::endl;
            return false;
        }

        const QVariant value = meta.value(key);

        if (!value.isValid()) {
            core::logErr() << "metadata missing key:" << key << Qt::endl;
            return false;
        }

        const QString stringValue = value.toString();

        if (stringValue != expected) {
            core::logErr() << "metadata mismatch for key:" << key << "expected:" << expected << "got:" << stringValue
                           << Qt::endl;
            return false;
        }

        return true;
    }

    bool testRect(const QRect& rect, int x, int y, int w, int h)
    {
        return testValue(rect.x(), x, "rect.x") && testValue(rect.y(), y, "rect.y")
               && testValue(rect.width(), w, "rect.width") && testValue(rect.height(), h, "rect.height");
    }
}  // namespace

void
init()
{
    QDir dir;
    dataPath = core::Environment::resourcePath("../../../data");
    if (!dir.exists(dataPath)) {
        core::logErr() << "data path does not exist:" << dataPath << Qt::endl;
    }

    const QString base = core::Environment::resourcePath("../../tests");
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    testPath = QString("%1/run_%2").arg(base, stamp);
    if (!dir.mkpath(testPath)) {
        core::logErr() << "failed to create test path:" << testPath << Qt::endl;
    }

    core::logOut() << Qt::endl;
    core::logOut() << "==================================================" << Qt::endl;
    core::logOut() << "running tests..." << Qt::endl;
    core::logOut() << "dataPath:" << dataPath << Qt::endl;
    core::logOut() << "testPath:" << testPath << Qt::endl;
    core::logOut() << "==================================================" << Qt::endl;
    core::logOut() << Qt::endl;
}

bool
testClip()
{
    core::logOut() << "test clip" << Qt::endl;

    QString filename = "timecode export 29 97 04-05-03_10.mov";

    av::Media media;
    core::File file = QString("%1/quicktime/%2").arg(dataPath).arg(filename);
    if (!file.exists()) {
        core::logOut() << "file does not exist: " << file << Qt::endl;
        return false;
    }

    core::logOut() << "file path: " << file << Qt::endl;
    if (!file.exists()) {
        core::logErr() << "file does not exist:" << file << Qt::endl;
        return false;
    }

    media.open(file);
    if (!media.waitForOpened(5000)) {
        core::logErr() << "timed out waiting for media to open:" << media.error().message() << Qt::endl;
        return false;
    }

    if (media.error().hasError()) {
        core::logErr() << "media error:" << media.error().message() << Qt::endl;
        return false;
    }

    bool ok = true;
    av::SmpteTime start = av::SmpteTime(media.start());
    ok &= start.hours() == 4;
    ok &= start.minutes() == 5;
    ok &= start.seconds() == 3;
    ok &= start.frames() == 10;
    if (!ok) {
        core::logErr() << "start time validation failed" << Qt::endl;
        return false;
    }

    const core::MetaData& meta = media.metaData();
    ok &= testMetaData(meta, "codec type", "avc1");
    ok &= testMetaData(meta, "media type", "vide");
    ok &= testMetaData(meta, "software", "Blackmagic Design DaVinci Resolve Studio");
    if (!ok) {
        core::logErr() << "metadata validation failed" << Qt::endl;
        return false;
    }

    av::Time time = media.read();
    ok &= time.isValid();
    if (!ok) {
        core::logErr() << "read validation failed" << Qt::endl;
        return false;
    }

    core::ImageBuffer image = media.image();
    ok &= testRect(image.dataWindow(), 0, 0, 1920, 1080);
    if (!ok) {
        core::logErr() << "datawindow validation failed" << Qt::endl;
        return false;
    }
    return true;
}

bool
testFile()
{
    core::logOut() << "test file" << Qt::endl;

    const QList<core::File> files = core::File::listDir(QString("%1/dir").arg(dataPath));

    for (const core::File& file : files) {
        core::logOut() << "file: " << file.displayName() << Qt::endl;
        const core::FileRange range = file.fileRange();
        if (range.isValid()) {
            core::logOut() << "frames: " << range.size() << "start: " << range.start() << "end: " << range.end()
                           << Qt::endl;
        }
    }

    const QStringList expected = { "23.967.[86400-86410].exr", "23.967.[86435-86440].exr", "24fps.[86400-86404].jpg",
                                   "24fps.[86514-86515].jpg", "square export 23.976.mov" };

    if (files.size() != expected.size()) {
        core::logErr() << "file count mismatch:"
                       << "expected: " << expected.size() << "got: " << files.size() << Qt::endl;
        return false;
    }

    for (int i = 0; i < expected.size(); ++i) {
        const QString actual = files[i].displayName();
        if (actual != expected[i]) {
            core::logErr() << "file sequence mismatch at index" << i << "expected:" << expected[i] << "got:" << actual
                           << Qt::endl;
            return false;
        }
    }

    return true;
}

bool
testFps()
{
    core::logOut() << "test fps" << Qt::endl;

    if (av::Fps::fps24() != av::Fps(24, 1)) {
        core::logErr() << "24 fps mismatch" << Qt::endl;
        return false;
    }

    core::logOut() << "fps 24: " << av::Fps::fps24().seconds() << Qt::endl;
    core::logOut() << "fps 24: " << av::Fps(24, 1).seconds() << Qt::endl;
    core::logOut() << "fps 24: " << (1.0 / 24.0) << Qt::endl;

    av::Time time;
    time.setTicks(24000 * 100);  // typical ticks, 100 seconds
    time.setTimeScale(24000);
    time.setFps(av::Fps::fps24());

    av::Fps guess23_97 = av::Fps::guess(23.976);
    if (!guess23_97.dropFrame()) {
        core::logErr() << "23.976 fps should use drop frame" << Qt::endl;
        return false;
    }
    core::logOut() << "fps 23.976: " << guess23_97 << Qt::endl;

    av::Fps guess24 = av::Fps::guess(24);
    if (guess24.dropFrame()) {
        core::logErr() << "24 fps should not use drop frame" << Qt::endl;
        return false;
    }
    core::logOut() << "fps 24: " << guess24 << Qt::endl;

    av::Fps guess10 = av::Fps::guess(10);
    if (guess10.dropFrame()) {
        core::logErr() << "10 fps should not use drop frame" << Qt::endl;
        return false;
    }
    core::logOut() << "fps 10: " << guess10 << Qt::endl;

    av::Fps fps23_97 = av::Fps::fps23_976();
    qint64 ticks = av::Time(time, fps23_97).ticks(1);
    if (ticks != 1001) {
        core::logErr() << "23.976 fps ticks mismatch: expected 1001, got " << ticks << Qt::endl;
        return false;
    }
    core::logOut() << "ticks 23_97: " << ticks << Qt::endl;

    av::Fps fps24 = av::Fps::fps24();
    ticks = av::Time(time, fps24).ticks(1);
    if (ticks != 1000) {
        core::logErr() << "24 fps ticks mismatch: expected 1000, got " << ticks << Qt::endl;
        return false;
    }
    core::logOut() << "ticks 24: " << ticks << Qt::endl;

    av::Fps fps29_97 = av::Fps::fps29_97();
    ticks = av::Time(time, fps29_97).ticks(2);
    if (ticks != 1602) {
        core::logErr() << "29.97 fps ticks(2) mismatch: expected 1602, got " << ticks << Qt::endl;
        return false;
    }
    core::logOut() << "ticks 29_97 (2): " << ticks << Qt::endl;

    ticks = av::Time(time, fps29_97).ticks(5);
    if (ticks != 4004) {
        core::logErr() << "29.97 fps ticks(5) mismatch: expected 4004, got " << ticks << Qt::endl;
        return false;
    }
    core::logOut() << "ticks 29_97 (5): " << ticks << Qt::endl;

    return true;
}

bool
testImageUint16()
{
    core::logOut() << "test image convert uint16" << Qt::endl;

    const QRect rect(0, 0, 1, 1);
    core::ImageBuffer src(rect, rect, core::ImageFormat(core::ImageFormat::UInt8), 3);

    quint8* s = src.data();
    s[0] = 255;  // R
    s[1] = 128;  // G
    s[2] = 0;    // B

    core::ImageBuffer dst = core::ImageBuffer::convert(src, core::ImageFormat::UInt16, 4);
    const quint16* d = reinterpret_cast<const quint16*>(dst.data());

    core::logOut() << "R:" << d[0] << " G:" << d[1] << " B:" << d[2] << " A:" << d[3] << Qt::endl;

    bool ok = true;

    if (d[0] != 65535) {
        core::logErr() << "R failed" << Qt::endl;
        ok = false;
    }

    if (d[1] != 128 * 257) {
        core::logErr() << "G failed" << Qt::endl;
        ok = false;
    }

    if (d[2] != 0) {
        core::logErr() << "B failed" << Qt::endl;
        ok = false;
    }
    core::logOut() << "alpha value is: " << d[3] << Qt::endl;

    core::logOut() << "image uint16 test passed" << Qt::endl;
    return ok;
}

bool
testImageDouble()
{
    core::logOut() << "test image convert double" << Qt::endl;

    const QRect rect(0, 0, 1, 1);
    core::ImageBuffer src(rect, rect, core::ImageFormat(core::ImageFormat::UInt8), 3);

    quint8* s = src.data();
    s[0] = 255;  // R
    s[1] = 128;  // G
    s[2] = 0;    // B

    core::ImageBuffer dst = core::ImageBuffer::convert(src, core::ImageFormat::Double, 3);
    const double* d = reinterpret_cast<const double*>(dst.data());

    core::logOut() << "R:" << d[0] << " G:" << d[1] << " B:" << d[2] << Qt::endl;

    const double eps = 1e-12;

    if (std::abs(d[0] - 1.0) > eps) {
        core::logErr() << "R failed" << Qt::endl;
        return false;
    }

    if (std::abs(d[1] - (128.0 / 255.0)) > eps) {
        core::logErr() << "G failed" << Qt::endl;
        return false;
    }

    if (std::abs(d[2] - 0.0) > eps) {
        core::logErr() << "B failed" << Qt::endl;
        return false;
    }

    return true;
}

bool
testImagePlanar()
{
    core::logOut() << "test image planar" << Qt::endl;

    QString imagePath = QString("%1/testImagePlanar").arg(testPath);
    QDir dir(imagePath);
    if (!dir.exists()) {
        core::logOut() << "path does not exist, creating: " << imagePath << Qt::endl;
        if (!dir.mkpath(".")) {
            core::logErr() << "failed to create directory: " << imagePath << Qt::endl;
            return false;
        }
    }

    const QRect dataWindow(0, 0, 1024, 1024);
    const QRect displayWindow = dataWindow;

    core::ImageFormat format(core::ImageFormat::UInt8);
    core::ImageBuffer planar(dataWindow, displayWindow, format, 3);
    planar.setPacking(core::ImageBuffer::Packing::Planar);
    planar.detach();

    if (planar.planeCount() != 3) {
        core::logErr() << "planar planeCount mismatch, expected 3 got " << planar.planeCount() << Qt::endl;
        return false;
    }

    const size_t planeStride0 = planar.planeStride(0);
    const size_t planeStride1 = planar.planeStride(1);
    const size_t planeStride2 = planar.planeStride(2);

    if (planeStride0 != planeStride1 || planeStride0 != planeStride2) {
        core::logErr() << "planar planeStride mismatch between planes" << Qt::endl;
        return false;
    }

    // planar plane stride = width * componentSize (UINT8 -> 1)
    const size_t expectedPlaneStride = size_t(dataWindow.width()) * planar.imageFormat().size();
    if (planeStride0 != expectedPlaneStride) {
        core::logErr() << "planar planeStride mismatch, expected " << expectedPlaneStride << " got " << planeStride0
                       << Qt::endl;
        return false;
    }

    quint8* rPlane = planar.planeData(0);
    quint8* gPlane = planar.planeData(1);
    quint8* bPlane = planar.planeData(2);

    if (!rPlane || !gPlane || !bPlane) {
        core::logErr() << "planar planeData returned null" << Qt::endl;
        return false;
    }

    if (rPlane == gPlane || rPlane == bPlane || gPlane == bPlane) {
        core::logErr() << "planar planes overlap (unexpected identical pointers)" << Qt::endl;
        return false;
    }

    // fill planar checkerboard: R plane checker, G inverted checker, B constant
    const int checker = 50;
    const size_t compSize = planar.imageFormat().size();  // 1 for UInt8
    for (int y = 0; y < dataWindow.height(); ++y) {
        const size_t rowOff = size_t(y) * planeStride0;
        for (int x = 0; x < dataWindow.width(); ++x) {
            const bool isWhite = ((x / checker) % 2) == ((y / checker) % 2);
            const quint8 r = isWhite ? 255 : 0;
            const quint8 g = isWhite ? 0 : 255;
            const quint8 b = 64;

            const size_t off = rowOff + size_t(x) * compSize;
            rPlane[off] = r;
            gPlane[off] = g;
            bPlane[off] = b;
        }
    }

    // convert planar to interleaved for visualization/saving (until ImageBuffer::convert supports planar directly)
    // build an interleaved RGB888 buffer and interleave manually.
    core::ImageBuffer planarRGB(dataWindow, displayWindow, format, 3);
    planarRGB.setPacking(core::ImageBuffer::Packing::Interleaved);
    planarRGB.detach();

    quint8* out = planarRGB.data();
    const size_t outStride = planarRGB.strideSize();
    const size_t outPixelSize = planarRGB.pixelSize();  // 3 bytes for UInt8 RGB

    for (int y = 0; y < dataWindow.height(); ++y) {
        const size_t inRow = size_t(y) * planeStride0;
        quint8* outRow = out + size_t(y) * outStride;

        for (int x = 0; x < dataWindow.width(); ++x) {
            const size_t inOff = inRow + size_t(x) * compSize;
            quint8* px = outRow + size_t(x) * outPixelSize;

            px[0] = rPlane[inOff];
            px[1] = gPlane[inOff];
            px[2] = bPlane[inOff];
        }
    }

    QImage planarImage(reinterpret_cast<const uchar*>(planarRGB.data()), dataWindow.width(), dataWindow.height(),
                       planarRGB.strideSize(), QImage::Format_RGB888);

    if (!planarImage.save(QString("%1/planarChecker.png").arg(imagePath))) {
        core::logErr() << "could not save planar_checker image" << Qt::endl;
        return false;
    }

    return true;
}

bool
testImageInterleaved()
{
    core::logOut() << "test image interleaved" << Qt::endl;

    QString imagePath = QString("%1/testImageInterleaved").arg(testPath);
    QDir dir(imagePath);
    if (!dir.exists()) {
        core::logOut() << "path does not exist, creating: " << imagePath << Qt::endl;
        if (!dir.mkpath(".")) {
            core::logErr() << "failed to create directory: " << imagePath << Qt::endl;
            return false;
        }
    }

    if (core::ImageFormat(core::ImageFormat::Half).size() != 2) {
        core::logErr() << "format size mismatch: HALF" << Qt::endl;
        return false;
    }
    if (core::ImageFormat(core::ImageFormat::Float).size() != 4) {
        core::logErr() << "format size mismatch: FLOAT" << Qt::endl;
        return false;
    }
    if (core::ImageFormat(core::ImageFormat::Double).size() != 8) {
        core::logErr() << "format size mismatch: DOUBLE" << Qt::endl;
        return false;
    }

    const QRect dataWindow(0, 0, 1000, 1000);
    const QRect displayWindow = dataWindow;

    core::ImageFormat format(core::ImageFormat::UInt8);
    core::ImageBuffer buffer(dataWindow, displayWindow, format, 3);

    quint8* data = buffer.data();
    const size_t stride = buffer.strideSize();
    const size_t pixelSize = buffer.pixelSize();

    const int checker = 50;
    for (int y = 0; y < dataWindow.height(); ++y) {
        for (int x = 0; x < dataWindow.width(); ++x) {
            const bool isWhite = ((x / checker) % 2) == ((y / checker) % 2);
            const quint8 color = isWhite ? 255 : 0;
            quint8* pixel = data + (y * stride) + (x * pixelSize);
            for (size_t c = 0; c < buffer.channels(); ++c) {
                pixel[c] = color;
            }
        }
    }

    QImage checkerImage(reinterpret_cast<uchar*>(buffer.data()), dataWindow.width(), dataWindow.height(),
                        buffer.strideSize(), QImage::Format_RGB888);

    if (!checkerImage.save(QString("%1/checker.png").arg(imagePath))) {
        core::logErr() << "could not save checker image" << Qt::endl;
        return false;
    }

    core::ImageBuffer ramp = buffer;
    ramp.detach();  // allocate new storage
    data = ramp.data();

    for (int y = 0; y < dataWindow.height(); ++y) {
        for (int x = 0; x < dataWindow.width(); ++x) {
            quint8* pixel = data + (y * stride) + (x * pixelSize);
            const float t = static_cast<float>(x) / dataWindow.width();
            pixel[0] = static_cast<quint8>(255 * (1.0f - t));
            pixel[1] = static_cast<quint8>(255 * t);
            pixel[2] = static_cast<quint8>(128 + 127 * std::sin(3.14f * t));
        }
    }

    QImage rampImage(reinterpret_cast<uchar*>(ramp.data()), dataWindow.width(), dataWindow.height(),
                     buffer.strideSize(), QImage::Format_RGB888);

    if (!rampImage.save(QString("%1/ramp.png").arg(imagePath))) {
        core::logErr() << "could not save ramp image" << Qt::endl;
        return false;
    }

    QImage checker2Image(reinterpret_cast<uchar*>(buffer.data()), dataWindow.width(), dataWindow.height(),
                         buffer.strideSize(), QImage::Format_RGB888);

    if (!checker2Image.save(QString("%1/checker2.png").arg(imagePath))) {
        core::logErr() << "could not save checker2 image" << Qt::endl;
        return false;
    }

    core::ImageBuffer ramp16 = core::ImageBuffer::convert(ramp, core::ImageFormat::UInt16, 4);
    QImage ramp16Image(reinterpret_cast<const uchar*>(ramp16.data()), dataWindow.width(), dataWindow.height(),
                       ramp16.strideSize(), QImage::Format_RGBX64);

    if (!ramp16Image.save(QString("%1/ramp16.png").arg(imagePath))) {
        core::logErr() << "could not save ramp16 image" << Qt::endl;
        return false;
    }

    return true;
}

bool
testImage()
{
    return testImageUint16() && testImageDouble() && testImagePlanar() && testImageInterleaved();
}

bool
testMedia()
{
    core::logOut() << "test media" << Qt::endl;
    QList<QString> filenames = { "quicktime/23 967 fps 24 fps timecode.mp4" };

    std::atomic<bool> ok { true };
    core::DispatchGroup group;
    for (const QString& filename : filenames) {
        group.async([filename, &ok]() {
            sdk::core::File file = QString("%1/%2").arg(dataPath).arg(filename);
            if (!file.exists()) {
                core::logErr() << "file does not exist: " << file << Qt::endl;
                ok = false;
                return;
            }

            sdk::av::Media media;
            if (!media.open(file)) {
                core::logErr() << "could not open media: " << file << ", error: " << media.error() << Qt::endl;
                ok = false;
                return;
            }

            if (!media.waitForOpened(-1)) {
                core::logErr() << "timed out waiting for media to open: " << media.error().message() << Qt::endl;
                ok = false;
                return;
            }

            QString imagePath = QString("%1/testMedia").arg(testPath);
            QDir dir(imagePath);
            if (!dir.exists()) {
                core::logOut() << "path does not exist, creating: " << imagePath << Qt::endl;
                if (!dir.mkpath(".")) {
                    core::logErr() << "failed to create directory: " << imagePath << Qt::endl;
                    ok = false;
                    return;
                }
            }

            sdk::core::File output = QString("%1/%2%3").arg(imagePath).arg(file.baseName()).arg(".#####.png");
            sdk::av::MediaProcessor mediaProcessor;

            QObject::connect(
                &mediaProcessor, &sdk::av::MediaProcessor::progressChanged, &mediaProcessor,
                [&file](const sdk::av::Time& time, const sdk::av::TimeRange& range) {
                    core::logOut() << "finished: " << file << Qt::endl
                                   << "time: " << time.toString() << Qt::endl
                                   << "frame: " << time.frames() << Qt::endl
                                   << "range: " << range.toString() << Qt::endl
                                   << Qt::endl;
                },
                Qt::DirectConnection);

            const sdk::av::TimeRange timeRange = media.timeRange();
            sdk::av::Time start = timeRange.start();
            sdk::av::Time requested(4.0, start.fps());
            sdk::av::Time duration = (requested < timeRange.duration()) ? requested : timeRange.duration();
            sdk::av::TimeRange writeRange(start, duration);

            if (!mediaProcessor.write(media, writeRange, output)) {
                core::logErr() << "could not process file: " << file << ", error: " << mediaProcessor.error()
                               << Qt::endl;
                ok = false;
                return;
            }
        });
    }
    group.wait();
    return ok.load();
}

bool
testRender()
{
    core::logOut() << "test render" << Qt::endl;
    QList<QString> filenames = { "quicktime/23 967 fps 24 fps timecode.mp4" };

    std::atomic<bool> ok { true };
    core::DispatchGroup group;
    for (const QString& filename : filenames) {
        group.async([filename, &ok]() {
            sdk::core::File file = QString("%1/%2").arg(dataPath).arg(filename);
            if (!file.exists()) {
                core::logErr() << "file does not exist: " << file << Qt::endl;
                ok = false;
                return;
            }

            sdk::av::Media media;
            if (!media.open(file)) {
                core::logErr() << "could not open media: " << file << ", error: " << media.error() << Qt::endl;
                ok = false;
                return;
            }

            if (!media.waitForOpened(-1)) {
                core::logErr() << "timed out waiting for media to open: " << media.error().message() << Qt::endl;
                ok = false;
                return;
            }

            av::Time time = media.read();
            if (!time.isValid()) {
                core::logErr() << "read failed" << Qt::endl;
                ok = false;
                return;
            }

            QString imagePath = QString("%1/testRender").arg(testPath);
            QDir dir(imagePath);
            if (!dir.exists()) {
                core::logOut() << "path does not exist, creating: " << imagePath << Qt::endl;
                if (!dir.mkpath(".")) {
                    core::logErr() << "failed to create directory: " << imagePath << Qt::endl;
                    ok = false;
                    return;
                }
            }

            sdk::core::File output = QString("%1/%2%3").arg(imagePath).arg(file.baseName()).arg(".#####.exr");
            QDir outDir(output.dirName());
            if (!outDir.exists()) {
                if (!outDir.mkpath(".")) {
                    core::logErr() << "could not create output directory:" << outDir.absolutePath() << Qt::endl;
                    ok = false;
                    return;
                }
            }

            render::RenderOffscreen renderOffscreen;
            if (!renderOffscreen.initialize(media.image().dataWindow().size())) {
                core::logErr() << "render offscreen initialization failed" << Qt::endl;
                ok = false;
                return;
            }

            renderOffscreen.setBackground(Qt::red);
            sdk::render::ImageLayer imageLayer;
            imageLayer.setImage(media.image());
            renderOffscreen.setImageLayers({ imageLayer });

            core::ImageBuffer image = renderOffscreen.render();

            if (!image.isValid()) {
                core::logErr() << "failed when trying to render image" << Qt::endl;
                ok = false;
                return;
            }

            QScopedPointer<plugins::MediaWriter> writer(
                core::pluginRegistry()->getPlugin<plugins::MediaWriter>(output.extension()));
            if (!writer) {
                core::logErr() << "no writer found for extension:" << output.extension() << Qt::endl;
                ok = false;
                return;
            }

            if (!writer->open(output)) {
                core::logErr() << "could not open writer for file:" << output << Qt::endl;
                ok = false;
                return;
            }

            sdk::av::TimeRange range = media.timeRange();
            writer->setTimeRange(range);
            if (!writer->write(image)) {
                core::logErr() << "could not write image" << Qt::endl;
                return;
            }

            // diff check
            {
                core::ImageBuffer rendered = core::ImageBuffer::convert(image, core::ImageFormat::UInt8, 4);
                core::ImageBuffer source = core::ImageBuffer::convert(media.image(), core::ImageFormat::UInt8, 4);

                if (rendered.dataWindow().size() != source.dataWindow().size()) {
                    core::logErr() << "image size mismatch" << Qt::endl;
                    ok = false;
                    return;
                }

                const int width = rendered.dataWindow().width();
                const int height = rendered.dataWindow().height();
                const size_t components = size_t(width) * height * 4;

                const quint8* a = rendered.data();
                const quint8* b = source.data();
                int diffCount = 0;

                core::ImageBuffer diff(rendered.dataWindow(), rendered.displayWindow(),
                                       core::ImageFormat(core::ImageFormat::UInt8), 4);

                quint8* d = diff.data();
                for (size_t i = 0; i < components; ++i) {
                    int v = std::abs(int(a[i]) - int(b[i]));
                    if (v > 1)
                        diffCount++;
                    d[i] = quint8(std::min(v * 10, 255));  // amplify diff for visibility
                }

                if (diffCount > 0) {
                    core::logErr() << "render mismatch detected, differing components:" << diffCount << Qt::endl;
                    sdk::core::File diffFile = QString("%1/%2_diff.exr").arg(imagePath).arg(file.baseName());

                    if (writer && writer->open(diffFile)) {
                        writer->setTimeRange(range);
                        writer->write(diff);
                    }
                    ok = false;
                }
            }
        });
    }
    group.wait();
    return ok.load();
}

bool
testShader()
{
    core::logOut() << "test media" << Qt::endl;

    QString filename = "fx/gaussian.fx";
    core::File file = QString("%1/%2").arg(dataPath).arg(filename);
    if (!file.exists()) {
        core::logOut() << "file does not exist: " << file << Qt::endl;
        return false;
    }

    render::ShaderComposer effectComposer;
    render::ShaderDefinition shaderDefinition = effectComposer.fromFile(file);
    if (!effectComposer.isValid()) {
        core::logErr() << "failed to interpret effect code: " << effectComposer.error().message() << Qt::endl;
        return false;
    }

    if (!shaderDefinition.isValid()) {
        core::logErr() << "shader definition is not valid" << Qt::endl;
        return false;
    }

    if (shaderDefinition.shaderCode().isEmpty()) {
        core::logErr() << "shaderCode is empty" << Qt::endl;
        return false;
    }

    if (shaderDefinition.uniformBlock().isEmpty()) {
        core::logErr() << "uniformBlock is empty (expected parameters?)" << Qt::endl;
        return false;
    }

    if (shaderDefinition.applyCode().isEmpty()) {
        core::logErr() << "applyCode is empty" << Qt::endl;
        return false;
    }

    if (shaderDefinition.shaderCode().contains("@param") || shaderDefinition.shaderCode().contains("@include")) {
        core::logErr() << "directives not stripped from shaderCode" << Qt::endl;
        return false;
    }

    const auto& params = shaderDefinition.descriptor().parameters;
    if (params.isEmpty()) {
        core::logErr() << "no parameters detected in descriptor" << Qt::endl;
        return false;
    }

    bool foundRadius = false;
    for (const auto& p : params) {
        if (p.name == "radius") {
            foundRadius = true;
            if (p.type != render::ShaderDefinition::ShaderParameter::Type::Float) {
                core::logErr() << "radius type mismatch" << Qt::endl;
                return false;
            }
            if (!p.defaultValue.isValid()) {
                core::logErr() << "radius default missing" << Qt::endl;
                return false;
            }
        }
    }

    if (!foundRadius) {
        core::logErr() << "expected parameter 'radius' missing" << Qt::endl;
        return false;
    }

    for (const auto& p : params) {
        if (!shaderDefinition.uniformBlock().contains(p.name)) {
            core::logErr() << "uniformBlock missing parameter:" << p.name << Qt::endl;
            return false;
        }
    }

    if (!shaderDefinition.shaderCode().contains("effect(")) {
        core::logErr() << "effect function missing in shaderCode" << Qt::endl;
        return false;
    }

    core::logOut() << "shader code: " << shaderDefinition.shaderCode() << Qt::endl;

    QFile viewerFile(":/flipmansdk/glsl/viewer.glsl");
    if (!viewerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        core::logErr() << "could not open viewer glsl" << Qt::endl;
        return false;
    }

    QByteArray viewerShader = viewerFile.readAll();

    QString idtCode = R"(vec3 idt(vec3 c)
{
    // simple log-like expansion
    return pow(max(c, vec3(0.0)), vec3(2.2));
})";

    QString odtCode =
        R"(vec3 odt(vec3 c)
{
    c = max(c, vec3(0.0));
    return pow(c, vec3(1.0 / 2.2));
})";

    render::ShaderComposer::Options options;
    options.injections.set("effectUniforms", shaderDefinition.uniformBlock());
    options.injections.set("effectCode", shaderDefinition.shaderCode());
    options.injections.set("effectApply", shaderDefinition.applyCode());
    options.injections.set("idtCode", idtCode);
    options.injections.set("odtCode", odtCode);
    options.injections.set("idtApply", "color.rgb = idt(color.rgb);");
    options.injections.set("odtApply", "color.rgb = odt(color.rgb);");

    render::ShaderComposer viewerComposer;
    render::ShaderDefinition viewerDefinition = viewerComposer.fromSource(viewerShader, options);
    if (!viewerComposer.isValid()) {
        core::logErr() << "failed to interpret viewer code: " << viewerComposer.error() << Qt::endl;
        return false;
    }

    if (viewerDefinition.shaderCode().isEmpty()) {
        core::logErr() << "viewerDefinition shaderCode empty" << Qt::endl;
        return false;
    }

    const QString viewerCode = viewerDefinition.shaderCode();
    if (viewerCode.isEmpty()) {
        core::logErr() << "viewer shaderCode is empty" << Qt::endl;
        return false;
    }

    if (viewerCode.contains("@")) {
        core::logErr() << "viewer shader still contains unresolved tokens" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("vec4 effect(")) {
        core::logErr() << "effect() function missing in viewer shader" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("vec3 idt(")) {
        core::logErr() << "idt() function missing in viewer shader" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("vec3 odt(")) {
        core::logErr() << "odt() function missing in viewer shader" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("color.rgb = idt(color.rgb);")) {
        core::logErr() << "idtApply missing in viewer shader" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("color = effect(color, vUV);")) {
        core::logErr() << "effectApply missing in viewer shader" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("color.rgb = odt(color.rgb);")) {
        core::logErr() << "odtApply missing in viewer shader" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("uniform EffectParams")) {
        core::logErr() << "EffectParams uniform block missing" << Qt::endl;
        return false;
    }

    if (!viewerCode.contains("float radius")) {
        core::logErr() << "radius uniform missing from viewer shader" << Qt::endl;
        return false;
    }

    core::logOut() << "shader code: " << viewerDefinition.shaderCode() << Qt::endl;

    QFile transformFile(":/flipmansdk/glsl/transform.glsl");
    if (!transformFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        core::logErr() << "could not open viewer glsl" << Qt::endl;
        return false;
    }

    QString transformCode = transformFile.readAll();

    render::ShaderCompiler compiler;
    render::ShaderCompiler::Options opts;
    opts.glslVersion = 440;
    opts.generateSpirv = true;
    opts.generateMsl = true;
    opts.generateHlsl = true;
    opts.optimize = true;

    QShader fragmentShader = compiler.compile(viewerDefinition.shaderCode(), QShader::FragmentStage, opts);
    if (!fragmentShader.isValid()) {
        core::logErr() << "fragment shader compilation failed: " << compiler.error().message() << Qt::endl;
        return false;
    }

    auto hasSource = [](const QShader& shader, QShader::Source src) {
        for (const QShaderKey& key : shader.availableShaders()) {
            if (key.source() == src)
                return true;
        }
        return false;
    };

    if (opts.generateSpirv && !hasSource(fragmentShader, QShader::SpirvShader)) {
        core::logErr() << "fragment shader missing SPIR-V variant" << Qt::endl;
        return false;
    }

    if (opts.generateMsl && !hasSource(fragmentShader, QShader::MslShader)) {
        core::logErr() << "fragment shader missing MSL variant" << Qt::endl;
        return false;
    }

    if (opts.generateHlsl && !hasSource(fragmentShader, QShader::HlslShader)) {
        core::logErr() << "fragment shader missing HLSL variant" << Qt::endl;
        return false;
    }

    const QShaderDescription shaderDescription = fragmentShader.description();
    if (!shaderDescription.isValid()) {
        core::logErr() << "fragment shader reflection invalid" << Qt::endl;
        return false;
    }

    bool foundEffectParams = false;
    QShaderDescription::UniformBlock effectBlock;

    for (const auto& block : shaderDescription.uniformBlocks()) {
        if (block.blockName == "EffectParams") {
            foundEffectParams = true;
            effectBlock = block;
            break;
        }
    }

    if (!foundEffectParams) {
        core::logErr() << "effectparams uniform block not found in reflection" << Qt::endl;
        return false;
    }

    for (const auto& param : params) {
        bool foundMember = false;
        for (const auto& member : effectBlock.members) {
            if (member.name == param.name) {
                foundMember = true;
                break;
            }
        }
        if (!foundMember) {
            core::logErr() << "parameter not found in reflected EffectParams block: " << param.name << Qt::endl;
            return false;
        }
    }

    QShader transformShader = compiler.compile(transformCode, QShader::VertexStage, opts);
    if (!transformShader.isValid()) {
        core::logErr() << "vertex shader compilation failed: " << compiler.error().message() << Qt::endl;
        return false;
    }

    return true;
}

bool
testSmpte()
{
    core::logOut() << "test smpte" << Qt::endl;

    av::Fps fps24 = av::Fps::fps24();
    qint64 frame = 86496;  // typical timecode, 01:00:04:00, 24 fps

    av::Time time(frame, fps24);
    if (!qFuzzyCompare(time.seconds(), 3604.0)) {
        core::logErr() << "86496 frames is not 3604 seconds, got " << time.seconds() << Qt::endl;
        return false;
    }
    core::logOut() << "time: " << time.seconds() << Qt::endl;

    qint64 frame_fps = frame;
    av::SmpteTime smpte(av::Time(frame_fps, av::Fps::fps24()));
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 (24 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps24(), av::Fps::fps50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps50()));
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 (50 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps50(), av::Fps::fps25());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps25()));
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 (25 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps25(), av::Fps::fps50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps50()));
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 (25 → 50 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps50(), av::Fps::fps23_976());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps23_976()));
    if (smpte.toString() != "01:00:04.00") {
        core::logErr() << "smpte is not 01:00:04.00 (23.976 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps23_976(), av::Fps::fps50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps50()));
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 (23.976 → 50 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps50(), av::Fps::fps24());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps24()));
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 (50 → 24 fps)" << Qt::endl;
        return false;
    }

    qint64 frame_df_23_976 = av::SmpteTime::convert(frame, av::Fps::fps23_976());
    qint64 frame_24 = av::SmpteTime::convert(frame_df_23_976, av::Fps::fps23_976(),
                                             true);  // reverse
    if (frame_df_23_976 != 86388) {
        core::logErr() << "86496 dropframe is not 86388, got " << frame_df_23_976 << Qt::endl;
        return false;
    }
    if (frame_24 != frame) {
        core::logErr() << "dropframe inverse does not match" << Qt::endl;
        return false;
    }

    smpte = av::SmpteTime(time);
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 (direct from Time)" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte 24 fps: " << smpte.toString() << Qt::endl;

    qint64 frame_30 = av::Fps::convert(frame, av::Fps::fps24(), av::Fps::fps30());
    smpte = av::SmpteTime(av::Time(frame_30, av::Fps::fps30()));
    if (smpte.toString() != "01:00:04:00") {
        core::logErr() << "smpte is not 01:00:04:00 for 30 fps" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte 30 fps: " << smpte.toString() << Qt::endl;

    qint64 frame_23_976 = av::SmpteTime::convert(frame_24, av::Fps::fps23_976());
    smpte = av::SmpteTime(av::Time(frame_23_976, av::Fps::fps23_976()));
    if (smpte.toString() != "01:00:04.00") {
        core::logErr() << "smpte is not 01:00:04.00 for 23.976 fps" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte 23.976 fps: " << smpte.toString() << Qt::endl;

    qint64 frame_29_997 = 440658;
    smpte = av::SmpteTime(av::Time(frame_29_997, av::Fps::fps29_97()));
    if (smpte.toString() != "04:05:03.10") {
        core::logErr() << "smpte is not 04:05:03.10 (29.97 fps)" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte 29_97 fps: " << smpte.toString() << Qt::endl;

    frame_29_997 = 442698;
    smpte = av::SmpteTime(av::Time(frame_29_997, av::Fps::fps29_97()));
    if (smpte.toString() != "04:06:11.12") {
        core::logErr() << "smpte is not 04:06:11.12 (29.97 fps)" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte 29_97 fps: " << smpte.toString() << Qt::endl;

    // quicktime data:
    // 01:00:04:00
    // 2542
    // 01:46
    // 01:01:49:22 (uses floor rather than round, 22583)
    // 23.976 fps
    time = av::Time(2544542, 24000, av::Fps::fps23_976());
    if (time.toString() != "01:46" || time.frames() != 2542) {
        core::logErr() << "QuickTime reference mismatch" << Qt::endl;
        return false;
    }
    core::logOut() << "time: " << time.toString() << Qt::endl;
    core::logOut() << "time frames: " << time.frames() << Qt::endl;

    frame = 2541;
    av::Time duration = av::Time(frame, av::Fps::fps23_976());  // 0–2542, 2541 last frame
    if (duration.frames() != frame) {
        core::logErr() << "frames mismatch for duration" << Qt::endl;
        return false;
    }
    core::logOut() << "time frames: " << duration.frames() << Qt::endl;

    frame = 86496;
    av::Time offset = av::Time(frame, av::Fps::fps24());  // typical timecode, 01:00:04:00, 24 fps
    core::logOut() << "offset max: " << offset.frames() << Qt::endl;
    core::logOut() << "offset smpte: " << av::SmpteTime(offset).toString() << Qt::endl;

    frame = av::SmpteTime::convert(offset.frames(),
                                   av::Fps::fps23_976());  // fix it to 01:00:04:00, match 23.976 timecode, drop frame
    if (frame != 86388) {
        core::logErr() << "drop frame is not 86388" << Qt::endl;
        return false;
    }
    core::logOut() << "offset dropframe: " << frame << Qt::endl;

    smpte = av::SmpteTime(av::Time(duration.frames() + frame, av::Fps::fps23_976()));
    if (smpte.toString() != "01:01:49.23") {
        core::logErr() << "smpte is not 01:01:49.23" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte: " << smpte.toString() << Qt::endl;

    // ffmpeg data:
    // time_base=1/24000
    // duration_ts=187903716
    // 7829.344000
    // 24000/1001
    // 2:10:29.344000
    time = av::Time(187903716, 24000, av::Fps::fps24());
    if (!qFuzzyCompare(time.seconds(), 7829.3215)) {
        core::logErr() << "seconds mismatch for ffmpeg reference" << Qt::endl;
        return false;
    }
    core::logOut() << "seconds: " << time.seconds() << Qt::endl;

    smpte = av::SmpteTime(time);
    if (smpte.toString() != "02:10:29:08") {  // *:08 equals 8/24 of a second = .344000
        core::logErr() << "smpte mismatch for ffmpeg reference" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte 24 fps: " << smpte.toString() << Qt::endl;

    // resolve data:
    // frame: 87040, converted to 87148 at fps: 23.967
    // 01:00:31:04
    // 01:00:30 (wall clock time)
    // 24 NDF is used in Resolve for 23.967 for timecode
    frame = 87040;
    time = av::Time(frame, av::Fps::fps23_976());
    if (time.toString() != "01:00:30") {
        core::logErr() << "Resolve wall clock mismatch" << Qt::endl;
        return false;
    }
    core::logOut() << "time: " << time.toString() << Qt::endl;

    smpte = av::SmpteTime(time);
    if (smpte.toString() != "01:00:31.04") {
        core::logErr() << "Resolve SMPTE mismatch" << Qt::endl;
        return false;
    }
    core::logOut() << "smpte 23.976: " << smpte.toString() << Qt::endl;

    return true;
}

bool
testTime()
{
    core::logOut() << "test time" << Qt::endl;

    av::Time time;
    time.setTicks(12000);
    time.setTimeScale(24000);
    time.setFps(av::Fps::fps24());

    if (time.tpf() != 1000) {
        core::logErr() << "ticks per frame mismatch: expected 1000, got " << time.tpf() << Qt::endl;
        return false;
    }

    if (time.frames() != 12) {
        core::logErr() << "ticks to frame mismatch: expected 12, got " << time.frames() << Qt::endl;
        return false;
    }

    if (time.ticks(12) != 12000) {
        core::logErr() << "frame to ticks mismatch: expected 12000, got " << time.ticks(12) << Qt::endl;
        return false;
    }

    core::logOut() << "ticks per frame: " << time.tpf() << Qt::endl;
    core::logOut() << "ticks frames: " << time.frames() << Qt::endl;
    core::logOut() << "frame to ticks: " << time.ticks(12) << Qt::endl;

    time.setTicks(16016);
    time.setTimeScale(30000);
    time.setFps(av::Fps::fps29_97());

    if (time.frames() != 16) {
        core::logErr() << "ticks to frame (29.97) mismatch: expected 16, got " << time.frames() << Qt::endl;
        return false;
    }

    core::logOut() << "ticks frames: " << time.frames() << Qt::endl;

    time = av::Time::convert(time, 24000);

    if (time.frames() != 16) {
        core::logErr() << "converted ticks to frame mismatch: expected 16, got " << time.frames() << Qt::endl;
        return false;
    }

    if (time.frame(time.ticks()) != 16) {
        core::logErr() << "frame(ticks) mismatch: expected 16, got " << time.frame(time.ticks()) << Qt::endl;
        return false;
    }

    if (time.align(time.ticks()) != time.ticks()) {
        core::logErr() << "ticks alignment failed" << Qt::endl;
        return false;
    }

    core::logOut() << "ticks: " << time.ticks() << Qt::endl;
    core::logOut() << "ticks frames: " << time.frames() << Qt::endl;

    time = av::Time::convert(time, 30000);

    if (time.ticks() != 16016) {
        core::logErr() << "ticks mismatch after reconversion: expected 16016, got " << time.ticks() << Qt::endl;
        return false;
    }

    core::logOut() << "ticks: " << time.ticks() << Qt::endl;

    time = av::Time(time.ticks() + time.ticks(1), time.timeScale(), time.fps());

    if (time.align(time.ticks()) != time.ticks()) {
        core::logErr() << "ticks alignment failed after increment" << Qt::endl;
        return false;
    }

    time.setTicks(8677230);
    time.setTimeScale(90000);
    time.setFps(av::Fps::fps23_976());

    core::logOut() << "frames: " << time.frames() << Qt::endl;
    core::logOut() << "ticks next frame: " << time.ticks(time.frames() + 1) << Qt::endl;

    time.setTicks(time.ticks(time.frames() + 1));

    core::logOut() << "frames after increment: " << time.frames() << Qt::endl;

    return true;
}

bool
testTimeRange()
{
    core::logOut() << "test time range" << Qt::endl;

    av::TimeRange timeRange;
    timeRange.setStart(av::Time(12000, 24000, av::Fps::fps24()));

    av::Time duration = av::Time::convert(av::Time(384000, 48000, av::Fps::fps24()), timeRange.start().timeScale());

    timeRange.setDuration(duration);

    if (duration.ticks() != 192000) {
        core::logErr() << "convert timescale failed: expected 192000 ticks, got " << duration.ticks() << Qt::endl;
        return false;
    }

    if (timeRange.end().ticks() != 204000) {
        core::logErr() << "end ticks mismatch: expected 204000, got " << timeRange.end().ticks() << Qt::endl;
        return false;
    }

    core::logOut() << "timerange: " << timeRange.toString() << Qt::endl;

    return true;
}

bool
testTimer()
{
    core::logOut() << "test timer" << Qt::endl;
    std::atomic<bool> ok { true };
    core::DispatchGroup group;
    group.async([&ok] {
        QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);

        av::Fps fps = av::Fps::fps23_976();
        qint64 start = 1;
        qint64 duration = 24 * 30;  // 30 secs
        av::TimeRange range(av::Time(start, fps), av::Time(duration, fps));

        av::Timer totalTimer;
        totalTimer.start();

        av::Timer timer;
        timer.start(fps);

        qint64 frames = range.duration().frames();
        qint64 dropped = 0;

        for (qint64 frame = range.start().frames(); frame < range.duration().frames(); ++frame) {
            quint64 currenttime = timer.elapsed();
            quint64 delay = QRandomGenerator::global()->bounded(1, 80);

            timer.sleep(delay);
            timer.wait();

            qreal elapsed = av::Timer::convert(timer.elapsed() - currenttime, av::Timer::Unit::Seconds);
            qreal deviation = elapsed - fps.seconds();
            core::logErr() << "frame[" << frame << "/" << frames << "]: " << elapsed << Qt::endl
                           << "deviation: " << deviation << Qt::endl
                           << "%: " << (deviation / fps.seconds()) * 100 << Qt::endl
                           << "delay: " << delay << Qt::endl
                           << Qt::endl;

            while (!timer.next(fps)) {
                ++frame;
                ++dropped;
                core::logErr() << "drop frame[" << frame << "] total dropped:" << dropped << Qt::endl;
            }
        }
        totalTimer.stop();

        qreal elapsed = av::Timer::convert(totalTimer.elapsed(), av::Timer::Unit::Seconds);
        qreal expected = frames * fps.seconds();
        qreal deviation = elapsed - expected;
        core::logErr() << "total elapsed: " << elapsed << Qt::endl
                       << "expected: " << expected << Qt::endl
                       << "deviation: " << deviation << Qt::endl
                       << "msecs: " << deviation * 1000 << Qt::endl
                       << "%: " << (deviation / expected) * 100 << Qt::endl
                       << "dropped: " << dropped << Qt::endl
                       << Qt::endl;

        if (qAbs(deviation) > 0.05) {
            core::logErr() << "FAIL: deviation more than 50 ms" << Qt::endl;
            ok = false;
        }
    });
    group.wait();
    return ok.load();
}

bool
testPluginFx()
{
    core::logOut() << "test plugin fx" << Qt::endl;

    QString filename = "fx/warm.fx";
    core::File file = QString("%1/%2").arg(dataPath).arg(filename);

    if (!file.exists()) {
        core::logErr() << "file does not exist: " << file << Qt::endl;
        return false;
    }

    std::atomic<bool> ok { true };
    core::DispatchGroup group;
    group.async([file, &ok]() {
        plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
        QScopedPointer<plugins::EffectReader> reader(registry->getPlugin<plugins::EffectReader>(file.extension()));
        if (!reader) {
            core::logErr() << "no reader found for extension: " << file.extension() << Qt::endl;
            ok = false;
            return;
        }
        if (!reader->open(file)) {
            core::logErr() << "open request rejected for file: " << file << Qt::endl;
            ok = false;
            return;
        }

        render::ImageEffect imageEffect = reader->imageEffect();
        if (!imageEffect.isValid()) {
            core::logErr() << "image effect is not valid" << Qt::endl;
            ok = false;
            return;
        }

        render::ShaderDefinition shaderDefinition = imageEffect.shaderDefinition();
        if (!shaderDefinition.isValid()) {
            core::logErr() << "shader definition is not valid" << Qt::endl;
            ok = false;
            return;
        }

        if (shaderDefinition.shaderCode().isEmpty()) {
            core::logErr() << "shaderCode is empty" << Qt::endl;
            ok = false;
            return;
        }

        if (shaderDefinition.uniformBlock().isEmpty()) {
            core::logErr() << "uniformBlock is empty (expected parameters?)" << Qt::endl;
            ok = false;
            return;
        }

        if (shaderDefinition.applyCode().isEmpty()) {
            core::logErr() << "applyCode is empty" << Qt::endl;
            ok = false;
            return;
        }

        if (shaderDefinition.shaderCode().contains("@param") || shaderDefinition.shaderCode().contains("@include")) {
            core::logErr() << "directives not stripped from shaderCode" << Qt::endl;
            ok = false;
            return;
        }

        const auto& params = shaderDefinition.descriptor().parameters;
        if (params.isEmpty()) {
            core::logErr() << "no parameters detected in descriptor" << Qt::endl;
            ok = false;
            return;
        }

        bool foundWarm = false;
        for (const auto& p : params) {
            if (p.name == "warm") {
                foundWarm = true;
                if (p.type != render::ShaderDefinition::ShaderParameter::Type::Float) {
                    core::logErr() << "radius type mismatch" << Qt::endl;
                    ok = false;
                    return;
                }
                if (!p.defaultValue.isValid()) {
                    core::logErr() << "radius default missing" << Qt::endl;
                    ok = false;
                    return;
                }
            }
        }

        if (!foundWarm) {
            core::logErr() << "expected parameter 'warm' missing" << Qt::endl;
            ok = false;
            return;
        }

        for (const auto& p : params) {
            if (!shaderDefinition.uniformBlock().contains(p.name)) {
                core::logErr() << "uniformBlock missing parameter:" << p.name << Qt::endl;
                ok = false;
                return;
            }
        }

        if (!shaderDefinition.shaderCode().contains("effect(")) {
            core::logErr() << "effect function missing in shaderCode" << Qt::endl;
            ok = false;
            return;
        }
    });
    group.wait();
    return ok.load();
}

bool
testPluginImage()
{
    core::logOut() << "test plugin image" << Qt::endl;
    QString filename = "quicktime/23 967 fps 24 fps timecode.mp4";
    core::File file = QString("%1/%2").arg(dataPath).arg(filename);
    if (!file.exists()) {
        core::logErr() << "file does not exist: " << file << Qt::endl;
        return false;
    }

    std::atomic<bool> ok { true };
    core::DispatchGroup group;
    group.async([file, &ok]() {
        plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
        QScopedPointer<plugins::MediaReader> reader(registry->getPlugin<plugins::MediaReader>(file.extension()));
        if (!reader) {
            core::logErr() << "no reader found for extension: " << file.extension() << Qt::endl;
            ok = false;
            return;
        }
        if (!reader->open(file)) {
            core::logErr() << "open request rejected for file: " << file << Qt::endl;
            ok = false;
            return;
        }
        if (!reader->isOpen()) {
            QEventLoop loop;
            QObject::connect(reader.data(), &plugins::MediaReader::opened, &loop, &QEventLoop::quit);
            loop.exec();
        }
        if (!reader->isOpen()) {
            core::logErr() << "could not open file: " << file << ", error: " << reader->error() << Qt::endl;
            ok = false;
            return;
        }

        qDebug() << "reader";
        qDebug() << " filename:" << file;

        sdk::av::TimeRange range = reader->timeRange();
        qDebug() << "timecode:";
        qDebug() << " start timecode:" << av::SmpteTime(reader->start() + range.start()).toString();
        qDebug() << " end timecode:" << av::SmpteTime(reader->start() + range.duration()).toString();

        qDebug() << "video:";
        qDebug() << " fps:" << reader->fps().real();
        qDebug() << " start frame:" << range.start().frames();
        qDebug() << " last frame:" << range.duration().lastFrame();
        qDebug() << " frames:" << range.duration().frames();

        qDebug() << "file:";
        qDebug() << " created:" << file.created().toString();
        qDebug() << " modified:" << file.modified().toString();

        if (reader->metaData().isValid()) {
            const auto meta = reader->metaData();
            qDebug() << "metadata:";
            for (auto group : meta.groups()) {
                const QString groupName = core::MetaData::convert(group);
                for (const QString& key : meta.keys(group)) {
                    qDebug() << " " << groupName + "::" + key << "=" << meta.value(group, key);
                }
            }
        }

        core::File output = core::Environment::resourcePath(QString("%1/testPlugins/qtwriter.#####.png").arg(testPath));

        QDir outDir(output.dirName());
        if (!outDir.exists()) {
            if (!outDir.mkpath(".")) {
                core::logErr() << "could not create output directory:" << outDir.absolutePath() << Qt::endl;
                ok = false;
                return;
            }
        }

        QScopedPointer<plugins::MediaWriter> writer(registry->getPlugin<plugins::MediaWriter>(output.extension()));
        if (!writer) {
            core::logErr() << "no writer found for extension:" << output.extension() << Qt::endl;
            ok = false;
            return;
        }

        if (!writer->open(output)) {
            core::logErr() << "could not open writer for file:" << output << Qt::endl;
            ok = false;
            return;
        }

        writer->setTimeRange(range);
        sdk::av::Time time = reader->seek(range);

        qint64 start = range.start().frames();
        qint64 end = range.end().frames();
        qint64 max = std::min<qint64>(100, end - start);
        for (qint64 frame = range.start().frames(); frame < max; ++frame) {
            qDebug() << "read frame:" << frame;
            av::Time next(frame, reader->fps());
            if (time < next || frame == range.start().frames()) {
                time = reader->read();
            }

            if (!writer->write(reader->image())) {
                core::logErr() << "could not write frame:" << frame << Qt::endl;
                ok = false;
                return;
            }
        }
    });
    group.wait();
    return ok.load();
}

bool
testPlugin()
{
    return testPluginFx() && testPluginImage();
}

bool
testPluginRegistry()
{
    core::logOut() << "test plugin registry" << Qt::endl;
    sdk::core::File file = QString("%1/quicktime/24fps.mov").arg(dataPath);

    if (!file.exists()) {
        core::logErr() << "file does not exist: " << file.filePath() << Qt::endl;
        return false;
    }

    core::DispatchGroup group;
    group.async([file]() {
        sdk::plugins::PluginRegistry* registry = sdk::plugins::PluginRegistry::instance();
        QScopedPointer<sdk::plugins::MediaReader> reader(
            registry->getPlugin<sdk::plugins::MediaReader>(file.extension()));

        if (!reader) {
            core::logErr() << "no reader found for extension: " << file.extension() << Qt::endl;
            return false;
        }

        if (!reader->open(file)) {
            core::logOut() << "file:" << file << "is open";
        }
        else {
            core::logErr() << "could not open file: " << reader->error() << Qt::endl;
        }
    });
    group.wait();
    return true;
}

bool
testTimeLine()
{
    core::logOut() << "test timeline" << Qt::endl;

    sdk::av::Timeline timeline;

    av::Time start(0.0, av::Fps::fps24());
    av::Time duration(10.0, av::Fps::fps24());  // 10 seconds
    av::TimeRange range(start, duration);

    bool ok = true;
    ok &= testValue(timeline.timeRange().start().frames(), static_cast<qint64>(0), "timerange start");
    ok &= testValue(timeline.timeRange().duration().frames(), static_cast<qint64>(0), "timerange duration");

    if (!ok) {
        core::logErr() << "timeline range handling failed" << Qt::endl;
        return false;
    }

    timeline.setTimeRange(range);

    av::Track* track1 = new av::Track(&timeline);
    av::Track* track2 = new av::Track(&timeline);

    timeline.insertTrack(track1);
    timeline.insertTrack(track2);

    ok &= testValue(timeline.tracks().size(), static_cast<qsizetype>(2), "track count");
    ok &= testValue(timeline.hasTrack(track1), true, "has track1");
    ok &= testValue(timeline.hasTrack(track2), true, "has track2");

    timeline.removeTrack(track1);
    ok &= testValue(timeline.tracks().size(), static_cast<qsizetype>(1), "track count after remove");
    ok &= testValue(timeline.hasTrack(track1), false, "track1 removed");

    if (!ok) {
        core::logErr() << "timeline track management failed" << Qt::endl;
        return false;
    }
    return true;
}
}  // namespace flipman::sdk::test
