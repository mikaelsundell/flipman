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

#include <flipmansdk/core/environment.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/filerange.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/system.h>

#include <flipmansdk/log/log.h>

#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/mediawriter.h>

#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/plugins/qt/qt.h>
#include <flipmansdk/plugins/quicktime/quicktime.h>

#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QtConcurrent>

#include <iostream>

namespace flipman::sdk::test {
namespace {
    QString resourcePath = "../../../data";
    template<typename T> bool testValue(const T& value, const T& expected, const char* label = nullptr)
    {
        if (value != expected) {
            if (label)
                log::err() << label << "mismatch:"
                           << "expected:" << expected << "got:" << value << Qt::endl;
            else
                log::err() << "value mismatch:"
                           << "expected:" << expected << "got:" << value << Qt::endl;
            return false;
        }
        return true;
    }

    bool testParameter(const core::Parameters& meta, const QString& key, const QString& expected)
    {
        if (!meta.keys().contains(key)) {
            log::err() << "metadata missing key:" << key << Qt::endl;
            return false;
        }

        const QString value = meta.value(key).toString();
        if (value != expected) {
            log::err() << "metadata mismatch for key:" << key << "expected:" << expected << "got:" << value << Qt::endl;
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

bool
testClip()
{
    log::out() << "Test clip" << Qt::endl;

    QString filename = "timecode export 29 97 04-05-03_10.mov";

    av::Media media;
    core::File file = QString("%1/%2").arg(core::Environment::resourcePath(resourcePath)).arg(filename);
    if (!file.exists()) {
        log::err() << "file does not exist: " << file << Qt::endl;
        return false;
    }

    log::out() << "file path: " << file << Qt::endl;
    if (!file.exists()) {
        log::err() << "file does not exist:" << file << Qt::endl;
        return false;
    }

    media.open(file);
    if (!media.waitForOpened(5000)) {
        log::err() << "timed out waiting for media to open:" << media.error().message() << Qt::endl;
        return false;
    }

    if (media.error().hasError()) {
        log::err() << "media error:" << media.error().message() << Qt::endl;
        return false;
    }

    bool ok = true;
    av::SmpteTime start = av::SmpteTime(media.start());
    ok &= start.hours() == 4;
    ok &= start.minutes() == 5;
    ok &= start.seconds() == 3;
    ok &= start.frames() == 10;
    if (!ok) {
        log::err() << "start time validation failed" << Qt::endl;
        return false;
    }

    const core::Parameters& meta = media.metaData();
    ok &= testParameter(meta, "codec type", "avc1");
    ok &= testParameter(meta, "media type", "vide");
    ok &= testParameter(meta, "title", filename);
    ok &= testParameter(meta, "software", "Blackmagic Design DaVinci Resolve Studio");
    if (!ok) {
        log::err() << "metadata validation failed" << Qt::endl;
        return false;
    }

    av::Time time = media.read();
    ok &= time.isValid();
    if (!ok) {
        log::err() << "read validation failed" << Qt::endl;
        return false;
    }

    core::ImageBuffer image = media.image();
    ok &= testRect(image.dataWindow(), 0, 0, 1920, 1080);
    if (!ok) {
        log::err() << "datawindow validation failed" << Qt::endl;
        return false;
    }
    return true;
}

bool
testFile()
{
    log::out() << "Test file" << Qt::endl;

    const QString path = QDir::cleanPath(QDir(core::Environment::applicationPath()).filePath(resourcePath));
    if (!QDir(path).exists()) {
        log::err() << "data does not exist:" << path << Qt::endl;
        return false;
    }

    const QList<core::File> files = core::File::listDir(QString("%1/%2").arg(path, "frames"));

    for (const core::File& file : files) {
        log::out() << "file: " << file.displayName() << Qt::endl;
        const core::FileRange range = file.fileRange();
        if (range.isValid()) {
            log::out() << "frames:" << range.size() << "start:" << range.start() << "end:" << range.end() << Qt::endl;
        }
    }

    const QStringList expected = { "23.967.[86400-86445].exr",
                                   "23.967.00086446exr.exr",
                                   "23.967.[86447-86456].exr",
                                   "23.967.[86458-86512].exr",
                                   "23.967.00086513exr.exr",
                                   "23.967.[86514-86522].exr",
                                   "23.967.00086523exr.exr",
                                   "23.967.[86524-86643].exr",
                                   "23.967.00086644exr.exr",
                                   "23.967.[86645-86660].exr",
                                   "23.96700086457.exr",
                                   "abc.exr",
                                   "test.[86400-86401].exr",
                                   "test.00086583exr.exr",
                                   "test.[86584-86585].exr",
                                   "24fps.[86400-86496].jpg",
                                   "24fps.00086497jpg.jpg",
                                   "24fps.[86498-86519].jpg",
                                   "23.967.tif" };

    if (files.size() != expected.size()) {
        log::err() << "file count mismatch:"
                   << "expected:" << expected.size() << "got:" << files.size() << Qt::endl;
        return false;
    }

    for (int i = 0; i < expected.size(); ++i) {
        const QString actual = files[i].displayName();
        if (actual != expected[i]) {
            log::err() << "file sequence mismatch at index" << i << "expected:" << expected[i] << "got:" << actual
                       << Qt::endl;
            return false;
        }
    }
    return true;
}

bool
testFps()
{
    log::out() << "Test fps" << Qt::endl;

    if (av::Fps::fps24() != av::Fps(24, 1)) {
        log::err() << "24 fps mismatch" << Qt::endl;
        return false;
    }

    log::out() << "fps 24: " << av::Fps::fps24().seconds() << Qt::endl;
    log::out() << "fps 24: " << av::Fps(24, 1).seconds() << Qt::endl;
    log::out() << "fps 24: " << (1.0 / 24.0) << Qt::endl;

    av::Time time;
    time.setTicks(24000 * 100);  // typical ticks, 100 seconds
    time.setTimeScale(24000);
    time.setFps(av::Fps::fps24());

    av::Fps guess23_97 = av::Fps::guess(23.976);
    if (!guess23_97.dropFrame()) {
        log::err() << "23.976 fps should use drop frame" << Qt::endl;
        return false;
    }
    log::out() << "fps 23.976: " << guess23_97 << Qt::endl;

    av::Fps guess24 = av::Fps::guess(24);
    if (guess24.dropFrame()) {
        log::err() << "24 fps should not use drop frame" << Qt::endl;
        return false;
    }
    log::out() << "fps 24: " << guess24 << Qt::endl;

    av::Fps guess10 = av::Fps::guess(10);
    if (guess10.dropFrame()) {
        log::err() << "10 fps should not use drop frame" << Qt::endl;
        return false;
    }
    log::out() << "fps 10: " << guess10 << Qt::endl;

    av::Fps fps23_97 = av::Fps::fps23_976();
    qint64 ticks = av::Time(time, fps23_97).ticks(1);
    if (ticks != 1001) {
        log::err() << "23.976 fps ticks mismatch: expected 1001, got " << ticks << Qt::endl;
        return false;
    }
    log::out() << "ticks 23_97: " << ticks << Qt::endl;

    av::Fps fps24 = av::Fps::fps24();
    ticks = av::Time(time, fps24).ticks(1);
    if (ticks != 1000) {
        log::err() << "24 fps ticks mismatch: expected 1000, got " << ticks << Qt::endl;
        return false;
    }
    log::out() << "ticks 24: " << ticks << Qt::endl;

    av::Fps fps29_97 = av::Fps::fps29_97();
    ticks = av::Time(time, fps29_97).ticks(2);
    if (ticks != 1602) {
        log::err() << "29.97 fps ticks(2) mismatch: expected 1602, got " << ticks << Qt::endl;
        return false;
    }
    log::out() << "ticks 29_97 (2): " << ticks << Qt::endl;

    ticks = av::Time(time, fps29_97).ticks(5);
    if (ticks != 4004) {
        log::err() << "29.97 fps ticks(5) mismatch: expected 4004, got " << ticks << Qt::endl;
        return false;
    }
    log::out() << "ticks 29_97 (5): " << ticks << Qt::endl;

    return true;
}

bool
testImage()
{
    log::out() << "Test image" << Qt::endl;

    if (core::ImageFormat(core::ImageFormat::HALF).size() != 2) {
        log::err() << "format size mismatch: HALF" << Qt::endl;
        return false;
    }
    if (core::ImageFormat(core::ImageFormat::FLOAT).size() != 4) {
        log::err() << "format size mismatch: FLOAT" << Qt::endl;
        return false;
    }
    if (core::ImageFormat(core::ImageFormat::DOUBLE).size() != 8) {
        log::err() << "format size mismatch: DOUBLE" << Qt::endl;
        return false;
    }

    const QRect dataWindow(0, 0, 1000, 1000);
    const QRect displayWindow = dataWindow;

    core::ImageFormat format(core::ImageFormat::UINT8);
    core::ImageBuffer buffer(dataWindow, displayWindow, format, 3);

    quint8* data = buffer.data();
    const size_t stride = buffer.strideSize();
    const size_t pixelSize = buffer.pixelSize();

    const QString testPath = core::Environment::resourcePath(QString("%1/test").arg(resourcePath));

    QDir dir(testPath);
    if (!dir.exists()) {
        log::out() << "path does not exist, creating: " << testPath << Qt::endl;
        if (!dir.mkpath(".")) {
            log::err() << "failed to create directory: " << testPath << Qt::endl;
            return false;
        }
    }

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

    if (!checkerImage.save(QString("%1/checker.png").arg(testPath))) {
        log::err() << "could not save checker image" << Qt::endl;
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

    if (!rampImage.save(QString("%1/ramp.png").arg(testPath))) {
        log::err() << "could not save ramp image" << Qt::endl;
        return false;
    }

    QImage checker2Image(reinterpret_cast<uchar*>(buffer.data()), dataWindow.width(), dataWindow.height(),
                         buffer.strideSize(), QImage::Format_RGB888);

    if (!checker2Image.save(QString("%1/checker2.png").arg(testPath))) {
        log::err() << "could not save checker2 image" << Qt::endl;
        return false;
    }

    core::ImageBuffer ramp16 = core::ImageBuffer::convert(ramp, core::ImageFormat::UINT16, 4);

    QImage ramp16Image(reinterpret_cast<const uchar*>(ramp16.data()), dataWindow.width(), dataWindow.height(),
                       ramp16.strideSize(), QImage::Format_RGBX64);

    if (!ramp16Image.save(QString("%1/ramp16.png").arg(testPath))) {
        log::err() << "could not save ramp16 image" << Qt::endl;
        return false;
    }

    return true;
}

bool
testMedia()
{
    log::out() << "Test media" << Qt::endl;

    QList<QString> filenames = {
        "23 967 fps 24 fps timecode.mp4" /*, "24fps.mov",
                                          "timecode export 23 976 01-30-13-23.mov", "timecode export 29 97 04-05-03_10.mov"*/
    };
    QList<QFuture<void>> futures;
    QFutureWatcher<void> watcher;
    for (const QString& filename : filenames) {
        QFuture<void> future = QtConcurrent::run([filename]() {
            sdk::core::File file
                = QString("%1/%2").arg(sdk::core::Environment::resourcePath(resourcePath)).arg(filename);
            if (!file.exists()) {
                log::err() << "file does not exist: " << file << Qt::endl;
                return false;
            }
            sdk::av::Media media;
            if (!media.open(file)) {
                log::err() << "could not open media: " << file << ", error: " << media.error() << Qt::endl;
            }

            if (!media.waitForOpened(-1)) {
                log::err() << "timed out waiting for media to open:" << media.error().message() << Qt::endl;
                return false;
            }

            sdk::core::File output = QString("%1/test/%2%3").arg(file.dirName()).arg(file.baseName()).arg(".#####.png");

            sdk::av::MediaProcessor processor;
            QObject::connect(
                &processor, &sdk::av::MediaProcessor::progressChanged, &processor,
                [&file](const sdk::av::Time& time, const sdk::av::TimeRange& range) {
                    log::out() << "processing: " << file << Qt::endl
                               << "time: " << time.toString() << Qt::endl
                               << "frame: " << time.frames() << Qt::endl
                               << "range: " << range.toString() << Qt::endl
                               << Qt::endl;
                },
                Qt::DirectConnection);

            sdk::av::TimeRange timerange = media.timeRange();
            if (processor.write(media, timerange, output)) {
                log::err() << "processed file: " << output << ", timerange: " << timerange.toString() << Qt::endl;
            }
            else {
                log::err() << "could not process file: " << file << ", error: " << processor.error() << Qt::endl;
            }
        });
        futures.append(future);
    }
    for (QFuture<void>& future : futures) {
        future.waitForFinished();
    }
    return true;
}

bool
testSmpte()
{
    log::out() << "Test smpte" << Qt::endl;

    av::Fps fps24 = av::Fps::fps24();
    qint64 frame = 86496;  // typical timecode, 01:00:04:00, 24 fps

    av::Time time(frame, fps24);
    if (!qFuzzyCompare(time.seconds(), 3604.0)) {
        log::err() << "86496 frames is not 3604 seconds, got " << time.seconds() << Qt::endl;
        return false;
    }
    log::out() << "time: " << time.seconds() << Qt::endl;

    qint64 frame_fps = frame;
    av::SmpteTime smpte(av::Time(frame_fps, av::Fps::fps24()));
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 (24 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps24(), av::Fps::fps50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps50()));
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 (50 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps50(), av::Fps::fps25());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps25()));
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 (25 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps25(), av::Fps::fps50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps50()));
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 (25 → 50 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps50(), av::Fps::fps23_976());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps23_976()));
    if (smpte.toString() != "01:00:04.00") {
        log::err() << "smpte is not 01:00:04.00 (23.976 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps23_976(), av::Fps::fps50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps50()));
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 (23.976 → 50 fps)" << Qt::endl;
        return false;
    }

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps50(), av::Fps::fps24());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps24()));
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 (50 → 24 fps)" << Qt::endl;
        return false;
    }

    qint64 frame_df_23_976 = av::SmpteTime::convert(frame, av::Fps::fps23_976());
    qint64 frame_24 = av::SmpteTime::convert(frame_df_23_976, av::Fps::fps23_976(),
                                             true);  // reverse
    if (frame_df_23_976 != 86388) {
        log::err() << "86496 dropframe is not 86388, got " << frame_df_23_976 << Qt::endl;
        return false;
    }
    if (frame_24 != frame) {
        log::err() << "dropframe inverse does not match" << Qt::endl;
        return false;
    }

    smpte = av::SmpteTime(time);
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 (direct from Time)" << Qt::endl;
        return false;
    }
    log::out() << "smpte 24 fps: " << smpte.toString() << Qt::endl;

    qint64 frame_30 = av::Fps::convert(frame, av::Fps::fps24(), av::Fps::fps30());
    smpte = av::SmpteTime(av::Time(frame_30, av::Fps::fps30()));
    if (smpte.toString() != "01:00:04:00") {
        log::err() << "smpte is not 01:00:04:00 for 30 fps" << Qt::endl;
        return false;
    }
    log::out() << "smpte 30 fps: " << smpte.toString() << Qt::endl;

    qint64 frame_23_976 = av::SmpteTime::convert(frame_24, av::Fps::fps23_976());
    smpte = av::SmpteTime(av::Time(frame_23_976, av::Fps::fps23_976()));
    if (smpte.toString() != "01:00:04.00") {
        log::err() << "smpte is not 01:00:04.00 for 23.976 fps" << Qt::endl;
        return false;
    }
    log::out() << "smpte 23.976 fps: " << smpte.toString() << Qt::endl;

    qint64 frame_29_997 = 440658;
    smpte = av::SmpteTime(av::Time(frame_29_997, av::Fps::fps29_97()));
    if (smpte.toString() != "04:05:03.10") {
        log::err() << "smpte is not 04:05:03.10 (29.97 fps)" << Qt::endl;
        return false;
    }
    log::out() << "smpte 29_97 fps: " << smpte.toString() << Qt::endl;

    frame_29_997 = 442698;
    smpte = av::SmpteTime(av::Time(frame_29_997, av::Fps::fps29_97()));
    if (smpte.toString() != "04:06:11.12") {
        log::err() << "smpte is not 04:06:11.12 (29.97 fps)" << Qt::endl;
        return false;
    }
    log::out() << "smpte 29_97 fps: " << smpte.toString() << Qt::endl;

    // quicktime data:
    // 01:00:04:00
    // 2542
    // 01:46
    // 01:01:49:22 (uses floor rather than round, 22583)
    // 23.976 fps
    time = av::Time(2544542, 24000, av::Fps::fps23_976());
    if (time.toString() != "01:46" || time.frames() != 2542) {
        log::err() << "QuickTime reference mismatch" << Qt::endl;
        return false;
    }
    log::out() << "time: " << time.toString() << Qt::endl;
    log::out() << "time frames: " << time.frames() << Qt::endl;

    frame = 2541;
    av::Time duration = av::Time(frame, av::Fps::fps23_976());  // 0–2542, 2541 last frame
    if (duration.frames() != frame) {
        log::err() << "frames mismatch for duration" << Qt::endl;
        return false;
    }
    log::out() << "time frames: " << duration.frames() << Qt::endl;

    frame = 86496;
    av::Time offset = av::Time(frame, av::Fps::fps24());  // typical timecode, 01:00:04:00, 24 fps
    log::out() << "offset max: " << offset.frames() << Qt::endl;
    log::out() << "offset smpte: " << av::SmpteTime(offset).toString() << Qt::endl;

    frame = av::SmpteTime::convert(offset.frames(),
                                   av::Fps::fps23_976());  // fix it to 01:00:04:00, match 23.976 timecode, drop frame
    if (frame != 86388) {
        log::err() << "drop frame is not 86388" << Qt::endl;
        return false;
    }
    log::out() << "offset dropframe: " << frame << Qt::endl;

    smpte = av::SmpteTime(av::Time(duration.frames() + frame, av::Fps::fps23_976()));
    if (smpte.toString() != "01:01:49.23") {
        log::err() << "smpte is not 01:01:49.23" << Qt::endl;
        return false;
    }
    log::out() << "smpte: " << smpte.toString() << Qt::endl;

    // ffmpeg data:
    // time_base=1/24000
    // duration_ts=187903716
    // 7829.344000
    // 24000/1001
    // 2:10:29.344000
    time = av::Time(187903716, 24000, av::Fps::fps24());
    if (!qFuzzyCompare(time.seconds(), 7829.3215)) {
        log::err() << "seconds mismatch for ffmpeg reference" << Qt::endl;
        return false;
    }
    log::out() << "seconds: " << time.seconds() << Qt::endl;

    smpte = av::SmpteTime(time);
    if (smpte.toString() != "02:10:29:08") {  // *:08 equals 8/24 of a second = .344000
        log::err() << "smpte mismatch for ffmpeg reference" << Qt::endl;
        return false;
    }
    log::out() << "smpte 24 fps: " << smpte.toString() << Qt::endl;

    // resolve data:
    // frame: 87040, converted to 87148 at fps: 23.967
    // 01:00:31:04
    // 01:00:30 (wall clock time)
    // 24 NDF is used in Resolve for 23.967 for timecode
    frame = 87040;
    time = av::Time(frame, av::Fps::fps23_976());
    if (time.toString() != "01:00:30") {
        log::err() << "Resolve wall clock mismatch" << Qt::endl;
        return false;
    }
    log::out() << "time: " << time.toString() << Qt::endl;

    smpte = av::SmpteTime(time);
    if (smpte.toString() != "01:00:31.04") {
        log::err() << "Resolve SMPTE mismatch" << Qt::endl;
        return false;
    }
    log::out() << "smpte 23.976: " << smpte.toString() << Qt::endl;

    return true;
}

bool
testTime()
{
    log::out() << "Test time" << Qt::endl;

    av::Time time;
    time.setTicks(12000);
    time.setTimeScale(24000);
    time.setFps(av::Fps::fps24());

    if (time.tpf() != 1000) {
        log::err() << "ticks per frame mismatch: expected 1000, got " << time.tpf() << Qt::endl;
        return false;
    }

    if (time.frames() != 12) {
        log::err() << "ticks to frame mismatch: expected 12, got " << time.frames() << Qt::endl;
        return false;
    }

    if (time.ticks(12) != 12000) {
        log::err() << "frame to ticks mismatch: expected 12000, got " << time.ticks(12) << Qt::endl;
        return false;
    }

    log::out() << "ticks per frame: " << time.tpf() << Qt::endl;
    log::out() << "ticks frames: " << time.frames() << Qt::endl;
    log::out() << "frame to ticks: " << time.ticks(12) << Qt::endl;

    time.setTicks(16016);
    time.setTimeScale(30000);
    time.setFps(av::Fps::fps29_97());

    if (time.frames() != 16) {
        log::err() << "ticks to frame (29.97) mismatch: expected 16, got " << time.frames() << Qt::endl;
        return false;
    }

    log::out() << "ticks frames: " << time.frames() << Qt::endl;

    time = av::Time::convert(time, 24000);

    if (time.frames() != 16) {
        log::err() << "converted ticks to frame mismatch: expected 16, got " << time.frames() << Qt::endl;
        return false;
    }

    if (time.frame(time.ticks()) != 16) {
        log::err() << "frame(ticks) mismatch: expected 16, got " << time.frame(time.ticks()) << Qt::endl;
        return false;
    }

    if (time.align(time.ticks()) != time.ticks()) {
        log::err() << "ticks alignment failed" << Qt::endl;
        return false;
    }

    log::out() << "ticks: " << time.ticks() << Qt::endl;
    log::out() << "ticks frames: " << time.frames() << Qt::endl;

    time = av::Time::convert(time, 30000);

    if (time.ticks() != 16016) {
        log::err() << "ticks mismatch after reconversion: expected 16016, got " << time.ticks() << Qt::endl;
        return false;
    }

    log::out() << "ticks: " << time.ticks() << Qt::endl;

    time = av::Time(time.ticks() + time.ticks(1), time.timeScale(), time.fps());

    if (time.align(time.ticks()) != time.ticks()) {
        log::err() << "ticks alignment failed after increment" << Qt::endl;
        return false;
    }

    time.setTicks(8677230);
    time.setTimeScale(90000);
    time.setFps(av::Fps::fps23_976());

    log::out() << "frames: " << time.frames() << Qt::endl;
    log::out() << "ticks next frame: " << time.ticks(time.frames() + 1) << Qt::endl;

    time.setTicks(time.ticks(time.frames() + 1));

    log::out() << "frames after increment: " << time.frames() << Qt::endl;

    return true;
}

bool
testTimeRange()
{
    log::out() << "Test time range" << Qt::endl;

    av::TimeRange timeRange;
    timeRange.setStart(av::Time(12000, 24000, av::Fps::fps24()));

    av::Time duration = av::Time::convert(av::Time(384000, 48000, av::Fps::fps24()), timeRange.start().timeScale());

    timeRange.setDuration(duration);

    if (duration.ticks() != 192000) {
        log::err() << "convert timescale failed: expected 192000 ticks, got " << duration.ticks() << Qt::endl;
        return false;
    }

    if (timeRange.end().ticks() != 204000) {
        log::err() << "end ticks mismatch: expected 204000, got " << timeRange.end().ticks() << Qt::endl;
        return false;
    }

    log::out() << "timerange: " << timeRange.toString() << Qt::endl;

    return true;
}

bool
testTimer()
{
    log::out() << "Test timer" << Qt::endl;

    std::atomic<bool> ok { true };
    QFuture<void> future = QtConcurrent::run([&ok] {
        QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);

        av::Fps fps = av::Fps::fps23_976();
        qint64 start = 1, duration = 24 * 30;  // 30 secs
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

            log::err() << "frame[" << frame << "/" << frames << "]: " << elapsed << Qt::endl
                       << "deviation: " << deviation << Qt::endl
                       << "%: " << (deviation / fps.seconds()) * 100 << Qt::endl
                       << "delay: " << delay << Qt::endl
                       << Qt::endl;

            while (!timer.next(fps)) {
                ++frame;
                ++dropped;
                log::err() << "drop frame[" << frame << "] total dropped:" << dropped << Qt::endl;
            }
        }

        totalTimer.stop();

        qreal elapsed = av::Timer::convert(totalTimer.elapsed(), av::Timer::Unit::Seconds);
        qreal expected = frames * fps.seconds();
        qreal deviation = elapsed - expected;

        log::err() << "total elapsed: " << elapsed << Qt::endl
                   << "expected: " << expected << Qt::endl
                   << "deviation: " << deviation << Qt::endl
                   << "msecs: " << deviation * 1000 << Qt::endl
                   << "%: " << (deviation / expected) * 100 << Qt::endl
                   << "dropped: " << dropped << Qt::endl
                   << Qt::endl;

        if (qAbs(deviation) > 0.05) {
            log::err() << "FAIL: deviation more than 50 ms" << Qt::endl;
            ok = false;
        }
    });

    future.waitForFinished();
    return ok.load();
}

bool
testPlugins()
{
    log::out() << "Test plugins" << Qt::endl;

    QString filename = "23 967 fps 24 fps timecode.mp4";

    core::File file = core::Environment::resourcePath(QString("%1/%2").arg(resourcePath).arg(filename));

    if (!file.exists()) {
        log::err() << "file does not exist: " << file << Qt::endl;
        return false;
    }

    std::atomic<bool> ok { true };

    QFuture<void> future = QtConcurrent::run([file, &ok]() {
        plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();

        QScopedPointer<plugins::MediaReader> reader(registry->getPlugin<plugins::MediaReader>(file.extension()));

        if (!reader) {
            log::err() << "no reader found for extension: " << file.extension() << Qt::endl;
            ok = false;
            return;
        }

        if (!reader->open(file)) {
            log::err() << "open request rejected for file: " << file << Qt::endl;
            ok = false;
            return;
        }

        if (!reader->isOpen()) {
            QEventLoop loop;
            QObject::connect(reader.data(), &plugins::MediaReader::opened, &loop, &QEventLoop::quit);
            loop.exec();
        }

        if (!reader->isOpen()) {
            log::err() << "could not open file: " << file << ", error: " << reader->error() << Qt::endl;
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
            qDebug() << "metadata:";
            for (const QString& key : reader->metaData().keys()) {
                qDebug() << " key:" << key << " value:" << reader->metaData().value(key);
            }
        }

        core::File output = core::Environment::resourcePath(
            QString("%1/test/testPlugins/qtwriter.#####.png").arg(resourcePath));

        QDir outDir(output.dirName());
        if (!outDir.exists()) {
            if (!outDir.mkpath(".")) {
                log::err() << "could not create output directory:" << outDir.absolutePath() << Qt::endl;
                ok = false;
                return;
            }
        }

        QScopedPointer<plugins::MediaWriter> writer(registry->getPlugin<plugins::MediaWriter>(output.extension()));

        if (!writer) {
            log::err() << "no writer found for extension:" << output.extension() << Qt::endl;
            ok = false;
            return;
        }

        if (!writer->open(output)) {
            log::err() << "could not open writer for file:" << output << Qt::endl;
            ok = false;
            return;
        }

        writer->setTimeRange(range);

        sdk::av::Time time = reader->seek(range);

        for (qint64 frame = range.start().frames(); frame < range.duration().frames(); ++frame) {
            qDebug() << "read frame:" << frame;

            av::Time next(frame, reader->fps());
            if (time < next || frame == range.start().frames()) {
                time = reader->read();
            }

            if (!writer->write(reader->image())) {
                log::err() << "could not write frame:" << frame << Qt::endl;
                ok = false;
                return;
            }
        }
    });

    future.waitForFinished();
    return ok.load();
}


bool
testPluginRegistry()
{
    sdk::core::File file = sdk::core::Environment::resourcePath("%1/24fps.mov").arg(resourcePath);
    Q_ASSERT("file does not exist" && file.exists());

    QFuture<void> future = QtConcurrent::run([file]() {  // run in thread when not called from timeline
        sdk::plugins::PluginRegistry* registry = sdk::plugins::PluginRegistry::instance();
        QScopedPointer<sdk::plugins::MediaReader> reader(
            registry->getPlugin<sdk::plugins::MediaReader>(file.extension()));
        Q_ASSERT("not reader found for extension" && reader);

        if (reader->open(file)) {
            qDebug() << "file: " << file << " is open";
        }
        else {
            qWarning() << "could not open file: " << reader->error();
        }
    });
    future.waitForFinished();
    return true;
}

bool
testTimeLine()
{
    sdk::av::Timeline timeline;
    return true;
}

}  // namespace flipman::sdk::test
