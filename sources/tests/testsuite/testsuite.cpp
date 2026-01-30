// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "testsuite.h"

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

#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/mediawriter.h>

#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/plugins/qt/qt.h>
#include <flipmansdk/plugins/quicktime/quicktime.h>

#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QtConcurrent>

namespace flipman {
namespace {
    QString resourcePath = "../../../data";
}

void
test_clip()
{
    qDebug() << "Testing clip";

    QString filename = "timecode export 29 97 04-05-03_10.mov";

    sdk::av::Clip clip;
    clip.setColor(Qt::red);
    clip.reset();

    sdk::av::Media media;
    sdk::core::File file = QString("%1/%2").arg(sdk::core::Environment::resourcePath(resourcePath)).arg(filename);
    qDebug() << "file " << file;
    
    Q_ASSERT("file does not exist" && file.exists());
    media.open(file);
    Q_ASSERT("media has error" && !media.error().hasError());

    clip.setPosition(0.5, 0.5);
    clip.setScale(0.5, 0.5);
    qDebug() << clip.transform();
}

void
test_file()
{
    qDebug() << "Testing file";
    qDebug() << "Program path:" << sdk::core::Environment::programPath();
    qDebug() << "Application path:" << sdk::core::Environment::applicationPath();

    QString data = QDir::cleanPath(QDir(sdk::core::Environment::applicationPath()).filePath(resourcePath));
    Q_ASSERT("data does not exist" && QDir(data).exists());

    QList<sdk::core::File> files = sdk::core::File::listDir(QString("%1/%2").arg(data).arg("frames"));

    for (sdk::core::File file : files) {
        qDebug() << file.displayName();
        sdk::core::FileRange range = file.fileRange();
        if (range.isValid()) {
            qDebug() << "- frames: " << range.size() << " start: " << range.start() << " end: " << range.end();
        }
    }

    Q_ASSERT(
        "file sequence does not match" && files[0].displayName() == "23.967.[86400-86445].exr"
        && files[1].displayName() == "23.967.00086446exr.exr" && files[2].displayName() == "23.967.[86447-86456].exr"
        && files[3].displayName() == "23.967.[86458-86512].exr" && files[4].displayName() == "23.967.00086513exr.exr"
        && files[5].displayName() == "23.967.[86514-86522].exr" && files[6].displayName() == "23.967.00086523exr.exr"
        && files[7].displayName() == "23.967.[86524-86643].exr" && files[8].displayName() == "23.967.00086644exr.exr"
        && files[9].displayName() == "23.967.[86645-86660].exr" && files[10].displayName() == "23.96700086457.exr"
        && files[11].displayName() == "abc.exr" && files[12].displayName() == "test.[86400-86401].exr"
        && files[13].displayName() == "test.00086583exr.exr" && files[14].displayName() == "test.[86584-86585].exr"
        && files[15].displayName() == "24fps.[86400-86496].jpg" && files[16].displayName() == "24fps.00086497jpg.jpg"
        && files[17].displayName() == "24fps.[86498-86519].jpg" && files[18].displayName() == "23.967.tif");
}

void
test_image()
{
    qDebug() << "Testing image";

    Q_ASSERT("format size half" && sdk::core::ImageFormat(sdk::core::ImageFormat::HALF).size() == 2);
    Q_ASSERT("format size float" && sdk::core::ImageFormat(sdk::core::ImageFormat::FLOAT).size() == 4);
    Q_ASSERT("format size double" && sdk::core::ImageFormat(sdk::core::ImageFormat::DOUBLE).size() == 8);

    QRect dataWindow(0, 0, 1000, 1000);
    QRect displayWindow = dataWindow;

    sdk::core::ImageFormat format(sdk::core::ImageFormat::UINT8);
    sdk::core::ImageBuffer buffer(dataWindow, displayWindow, format, 3);

    quint8* data = buffer.data();
    size_t stride = buffer.strideSize();
    size_t pixelsize = buffer.pixelSize();

    QString testpath = sdk::core::Environment::resourcePath(QString("%1/test_image").arg(resourcePath));
    QDir dir(testpath);
    if (!dir.exists()) {
        qDebug() << "path does not exist. Creating:" << testpath;
        Q_ASSERT(dir.mkpath("."));
    }

    const int checker = 50;
    for (int y = 0; y < dataWindow.height(); ++y) {
        for (int x = 0; x < dataWindow.width(); ++x) {
            bool iswhite = ((x / checker) % 2) == ((y / checker) % 2);
            quint8 color = iswhite ? 255 : 0;
            quint8* pixel = data + (y * stride) + (x * pixelsize);
            for (size_t c = 0; c < buffer.channels(); ++c) {
                pixel[c] = color;
            }
        }
    }
    QImage checkerimage(reinterpret_cast<uchar*>(buffer.data()), dataWindow.width(), dataWindow.height(),
                        buffer.strideSize(), QImage::Format_RGB888);

    Q_ASSERT("could not save checker image" && checkerimage.save(QString("%1/%2").arg(testpath).arg("checker.png")));

    sdk::core::ImageBuffer ramp = buffer;
    ramp.detach();  // detach and alloc new
    data = ramp.data();
    for (int y = 0; y < dataWindow.height(); ++y) {
        for (int x = 0; x < dataWindow.width(); ++x) {
            quint8* pixel = data + (y * stride) + (x * pixelsize);
            float t = static_cast<float>(x) / dataWindow.width();
            pixel[0] = static_cast<quint8>(255 * (1.0 - t));
            pixel[1] = static_cast<quint8>(255 * t);
            pixel[2] = static_cast<quint8>(128 + 127 * sin(3.14 * t));
        }
    }

    QImage rampImage(reinterpret_cast<uchar*>(ramp.data()), dataWindow.width(), dataWindow.height(),
                     buffer.strideSize(), QImage::Format_RGB888);

    Q_ASSERT("could not save ramp image" && rampImage.save(QString("%1/%2").arg(testpath).arg("ramp.png")));

    QImage checker2image(reinterpret_cast<uchar*>(buffer.data()), dataWindow.width(), dataWindow.height(),
                         buffer.strideSize(), QImage::Format_RGB888);

    Q_ASSERT("could not save checker2 image, should match checker image"
             && checker2image.save(QString("%1/%2").arg(testpath).arg("checker2.png")));

    sdk::core::ImageBuffer ramp16 = sdk::core::ImageBuffer::convert(ramp, sdk::core::ImageFormat::UINT16, 4);
    QImage ramp16image(reinterpret_cast<const uchar*>(ramp16.data()), dataWindow.width(), dataWindow.height(),
                       ramp16.strideSize(), QImage::Format_RGBX64);

    Q_ASSERT("could not save ramp16 image" && ramp16image.save(QString("%1/%2").arg(testpath).arg("ramp16.png")));
}

void
test_time()
{
    qDebug() << "Testing time";

    sdk::av::Time time;
    time.setTicks(12000);
    time.setTimeScale(24000);
    time.setFps(sdk::av::Fps::fps24());
    Q_ASSERT("ticks per frame" && time.tpf() == 1000);
    Q_ASSERT("ticks to frame" && time.frames() == 12);
    Q_ASSERT("frame to ticks" && time.ticks(12) == 12000);

    qDebug() << "ticks per frame: " << time.tpf();
    qDebug() << "ticks frames: " << time.frames();
    qDebug() << "frame to ticks: " << time.ticks(12);

    time.setTicks(16016);
    time.setTimeScale(30000);
    time.setFps(sdk::av::Fps::fps29_97());
    Q_ASSERT("ticks to frame" && time.frames() == 16);

    qDebug() << "ticks frames: " << time.frames();

    time = sdk::av::Time::convert(time, 24000);
    Q_ASSERT("ticks to frame" && time.frames() == 16);
    Q_ASSERT("ticks to frame" && time.frame(time.ticks()) == 16);
    Q_ASSERT("ticks align" && time.align(time.ticks()) == time.ticks());

    qDebug() << "ticks: " << time.ticks();
    qDebug() << "ticks frames: " << time.frames();

    time = sdk::av::Time::convert(time, 30000);
    qDebug() << "ticks: " << time.ticks();
    Q_ASSERT("ticks" && time.ticks() == 16016);

    time = sdk::av::Time(time.ticks() + time.ticks(1), time.timeScale(), time.fps());
    Q_ASSERT("ticks align" && time.align(time.ticks()) == time.ticks());

    time.setTicks(8677230);
    time.setTimeScale(90000);
    time.setFps(sdk::av::Fps::fps23_976());

    qDebug() << "frames: " << time.frames();
    qDebug() << "ticks: " << time.ticks(time.frames() + 1);

    time.setTicks(time.ticks(time.frames() + 1));
    qDebug() << "frames: " << time.frames();
}

void
test_timerange()
{
    sdk::av::TimeRange timeRange;
    timeRange.setStart(sdk::av::Time(12000, 24000, sdk::av::Fps::fps24()));
    sdk::av::Time duration = sdk::av::Time::convert(sdk::av::Time(384000, 48000, sdk::av::Fps::fps24()), timeRange.start().timeScale());
    timeRange.setDuration(duration);
    Q_ASSERT("convert timescale" && duration.ticks() == 192000);
    Q_ASSERT("end ticks" && timeRange.end().ticks() == 204000);

    qDebug() << "timerange: " << timeRange.toString();
}

void
test_fps()
{
    qDebug() << "Testing fps";

    Q_ASSERT("24 fps" && sdk::av::Fps::fps24() == sdk::av::Fps(24, 1));
    qDebug() << "fps 24: " << sdk::av::Fps::fps24().seconds();
    qDebug() << "fps 24: " << sdk::av::Fps(24, 1).seconds();
    qDebug() << "fps 24: " << 1 / 24.0;

    sdk::av::Time time;
    time.setTicks(24000 * 100);  // typical ticks, 100 seconds
    time.setTimeScale(24000);
    time.setFps(sdk::av::Fps::fps24());
    qint64 ticks;

    sdk::av::Fps guess23_97 = sdk::av::Fps::guess(23.976);
    Q_ASSERT("23.976 fps has drop frames" && guess23_97.dropFrame());
    qDebug() << "fps 23.976: " << guess23_97;

    sdk::av::Fps guess24 = sdk::av::Fps::guess(24);
    Q_ASSERT("24 fps is standard" && !guess24.dropFrame());
    qDebug() << "fps 24: " << guess24;

    sdk::av::Fps guess10 = sdk::av::Fps::guess(10);
    Q_ASSERT("10 fps is standard" && !guess10.dropFrame());
    qDebug() << "fps 10: " << guess10;

    sdk::av::Fps fps23_97 = sdk::av::Fps::fps23_976();
    ticks = sdk::av::Time(time, fps23_97).ticks(1);
    Q_ASSERT("23.97 fps ticks" && ticks == 1001);
    qDebug() << "ticks 23_97: " << ticks;

    sdk::av::Fps fps24 = sdk::av::Fps::fps24();
    ticks = sdk::av::Time(time, fps24).ticks(1);
    Q_ASSERT("24 fps ticks" && ticks == 1000);
    qDebug() << "ticks 24: " << ticks;

    sdk::av::Fps fps29_97 = sdk::av::Fps::fps29_97();
    ticks = sdk::av::Time(time, fps29_97).ticks(2);
    qDebug() << "ticks 29_97: " << ticks;
    Q_ASSERT("29.97 fps ticks" && ticks == 1602);

    ticks = sdk::av::Time(time, fps29_97).ticks(5);
    qDebug() << "ticks 29_97: " << ticks;
    Q_ASSERT("29.97 fps ticks" && ticks == 4004);
}

void
test_smpte()
{
    qDebug() << "Testing SMPTE";

    sdk::av::Fps fps_24 = sdk::av::Fps::fps24();
    qint64 frame = 86496;  // typical timecode, 01:00:04:00, 24 fps
    sdk::av::Time time(frame, fps_24);
    Q_ASSERT("86496 frames is 3604" && qFuzzyCompare(time.seconds(), 3604));
    qDebug() << "time: " << time.seconds();

    qint64 frame_fps = frame;
    sdk::av::SmpteTime smpte(sdk::av::Time(frame_fps, sdk::av::Fps::fps24()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04:00");

    frame_fps = sdk::av::SmpteTime::convert(frame_fps, sdk::av::Fps::fps24(), sdk::av::Fps::fps50());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_fps, sdk::av::Fps::fps50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04:00");

    frame_fps = sdk::av::SmpteTime::convert(frame_fps, sdk::av::Fps::fps50(), sdk::av::Fps::fps25());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_fps, sdk::av::Fps::fps25()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04:00");

    frame_fps = sdk::av::SmpteTime::convert(frame_fps, sdk::av::Fps::fps25(), sdk::av::Fps::fps50());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_fps, sdk::av::Fps::fps50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04:00");

    frame_fps = sdk::av::SmpteTime::convert(frame_fps, sdk::av::Fps::fps50(), sdk::av::Fps::fps23_976());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_fps, sdk::av::Fps::fps23_976()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04.00");

    frame_fps = sdk::av::SmpteTime::convert(frame_fps, sdk::av::Fps::fps23_976(), sdk::av::Fps::fps50());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_fps, sdk::av::Fps::fps50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04:00");

    frame_fps = sdk::av::SmpteTime::convert(frame_fps, sdk::av::Fps::fps50(), sdk::av::Fps::fps24());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_fps, sdk::av::Fps::fps24()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04:00");

    qint64 frame_df_23_976 = sdk::av::SmpteTime::convert(frame, sdk::av::Fps::fps23_976());
    qint64 frame_24 = sdk::av::SmpteTime::convert(frame_df_23_976, sdk::av::Fps::fps23_976(),
                                             true);  // reverse
    Q_ASSERT("86496 dropframe is 86388" && frame_df_23_976 == 86388);
    Q_ASSERT("dropframe inverse does not match" && frame == frame_24);

    smpte = sdk::av::SmpteTime(time);
    Q_ASSERT("smpte is 01:00:04:00" && smpte.toString() == "01:00:04:00");
    qDebug() << "smpte 24 fps: " << smpte.toString();

    qint64 frame_30 = sdk::av::Fps::convert(frame, sdk::av::Fps::fps24(), sdk::av::Fps::fps30());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_30, sdk::av::Fps::fps30()));
    Q_ASSERT("smpte is 01:00:04:00 for 30 fps" && smpte.toString() == "01:00:04:00");
    qDebug() << "smpte 30 fps: " << smpte.toString();

    qint64 frame_23_976 = sdk::av::SmpteTime::convert(frame_24, sdk::av::Fps::fps23_976());
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_23_976, sdk::av::Fps::fps23_976()));
    Q_ASSERT("smpte is 01:00:04.00 for 23.976 fps" && smpte.toString() == "01:00:04.00");
    qDebug() << "smpte 23.976 fps: " << smpte.toString();

    qint64 frame_29_997 = 440658;
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_29_997, sdk::av::Fps::fps29_97()));
    Q_ASSERT("smpte is 04:05:03.10" && smpte.toString() == "04:05:03.10");
    qDebug() << "smpte 29_97 fps: " << smpte.toString();

    frame_29_997 = 442698;
    smpte = sdk::av::SmpteTime(sdk::av::Time(frame_29_997, sdk::av::Fps::fps29_97()));
    Q_ASSERT("smpte is 04:06:11.12" && smpte.toString() == "04:06:11.12");
    qDebug() << "smpte 29_97 fps: " << smpte.toString();

    // quicktime data:
    // 01:00:04:00
    // 2542
    // 01:46
    // 01:01:49:22 (uses floor rather than round, 22583)
    // 23.976 fps
    time = sdk::av::Time(2544542, 24000, sdk::av::Fps::fps23_976());
    Q_ASSERT("time is 01:46" && time.toString() == "01:46");
    Q_ASSERT("frames is 2542" && time.frames() == 2542);
    qDebug() << "time: " << time.toString();
    qDebug() << "time frames: " << time.frames();

    frame = 2541;
    sdk::av::Time duration = sdk::av::Time(frame, sdk::av::Fps::fps23_976());  // 0-2542, 2541 last frame
    Q_ASSERT("frames is 2541" && duration.frames() == frame);
    qDebug() << "time frames: " << duration.frames();

    frame = 86496;
    sdk::av::Time offset = sdk::av::Time(frame,
                               sdk::av::Fps::fps24());  // typical timecode, 01:00:04:00, 24 fps
    qDebug() << "offset max: " << offset.frames();
    qDebug() << "offset smpte: " << sdk::av::SmpteTime(offset).toString();

    frame = sdk::av::SmpteTime::convert(offset.frames(),
                                   sdk::av::Fps::fps23_976());  // fix it to 01:00:04:00, match 23.976 timecode, drop frame
    Q_ASSERT("drop frame is 86388" && frame == 86388);
    qDebug() << "offset dropframe: " << frame;

    smpte = sdk::av::SmpteTime(sdk::av::Time(duration.frames() + frame, sdk::av::Fps::fps23_976()));
    Q_ASSERT("smpte is 01:01:49.23" && smpte.toString() == "01:01:49.23");
    qDebug() << "smpte: " << smpte.toString();

    // ffmpeg data:
    // time_base=1/24000
    // duration_ts=187903716
    // 7829.344000
    // 24000/1001
    // 2:10:29.344000
    time = sdk::av::Time(187903716, 24000,
                    sdk::av::Fps::fps24());  // typical duration, 02:10:29.344000, 24 fps
    Q_ASSERT("seconds 7829.32" && qFuzzyCompare(time.seconds(), 7829.3215));
    qDebug() << "seconds: " << time.seconds();

    smpte = sdk::av::SmpteTime(time);
    Q_ASSERT("smpte is 02:10:29:08 for 24 fps"
             && smpte.toString() == "02:10:29:08");  // *:08 equals 8/24 of a second = .344000
    qDebug() << "smpte 24 fps: " << smpte.toString();

    // resolve data:
    // frame: 87040, converted to 87148 at fps: 23.967
    // 01:00:31:04
    // 01:00:30 (wall clock time)
    // 24 NDF is used in Resolve for 23.967 for timecode
    frame = 87040;
    time = sdk::av::Time(frame, sdk::av::Fps::fps23_976());
    Q_ASSERT("time is 01:00:30" && time.toString() == "01:00:30");
    qDebug() << "time: " << time.toString();

    smpte = sdk::av::SmpteTime(time);
    Q_ASSERT("smpte is 01:00:31.04 for 23.976 fps" && smpte.toString() == "01:00:31.04");
    qDebug() << "smpte 23.976: " << smpte.toString();
}

void
test_media()
{
    qDebug() << "Testing media";

    QList<QString> filenames = { "23 967 fps 24 fps timecode.mp4", "24fps.mov",
                                 "timecode export 23 976 01-30-13-23.mov", "timecode export 29 97 04-05-03_10.mov" };
    QList<QFuture<void>> futures;
    QFutureWatcher<void> watcher;
    for (const QString& filename : filenames) {
        QFuture<void> future = QtConcurrent::run([filename]() {
            sdk::core::File file = QString("%1/%2").arg(sdk::core::Environment::resourcePath(resourcePath)).arg(filename);
            Q_ASSERT("file does not exist" && file.exists());
            sdk::av::Media media;
            if (media.open(file)) {
                sdk::core::File output = QString("%1/test/%2%3").arg(file.dirName()).arg(file.baseName()).arg(".#####.png");
                sdk::av::MediaProcessor processor;

                QObject::connect(&processor, &sdk::av::MediaProcessor::progressChanged,
                                 [&file](const sdk::av::Time& time, const sdk::av::TimeRange& range) {
                                     qDebug() << "processing:" << file << " - time:" << time.toString()
                                              << ", frame: " << time.frames() << ", timerange: " << range.toString();
                                 });
                sdk::av::TimeRange timerange = media.timeRange();
                if (processor.write(media, timerange, output)) {
                    qDebug() << "processed file: " << output << ", timerange: " << timerange.toString();
                }
                else {
                    qDebug() << "could not process file: " << file << ", error: " << processor.error();
                }
            }
            else {
                qDebug() << "could not open media: " << file << ", error: " << media.error();
            }
        });
        futures.append(future);
    }
    for (QFuture<void>& future : futures) {
        future.waitForFinished();
    }
}

void
test_timer()
{
    qDebug() << "Testing timer";

    QFuture<void> future = QtConcurrent::run([] {  // run in thread when not called from timeline
        QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
        sdk::av::Fps fps = sdk::av::Fps::fps23_976();
        qint64 start = 1, duration = 24 * 400;
        sdk::av::TimeRange range(sdk::av::Time(start, fps), sdk::av::Time(duration, fps));
        sdk::av::Timer totalTimer;
        totalTimer.start();
        sdk::av::Timer timer;
        timer.start(fps);

        qDebug() << "range: start:" << range.start().frames() << ", duration: " << range.duration().frames();
        qint64 frames = range.duration().frames();

        qint64 dropped = 0;
        for (qint64 frame = range.start().frames(); frame < range.duration().frames(); frame++) {
            quint64 currenttime = timer.elapsed();
            quint64 delay = QRandomGenerator::global()->bounded(1, 80);
            timer.sleep(delay);
            timer.wait();

            qreal elapsed = sdk::av::Timer::convert(timer.elapsed() - currenttime, sdk::av::Timer::Unit::Seconds);
            qreal deviation = elapsed - fps.seconds();

            qDebug() << "frame[" << frame << "/" << frames << "]:" << elapsed << "|"
                     << "deviation:" << deviation << ","
                     << "%:" << (deviation / fps.seconds()) * 100 << ", delay: " << delay;

            while (!timer.next(fps)) {
                frame++;
                dropped++;
                qDebug() << "drop frame[" << frame << "] total frames dropped: " << dropped
                         << ", previous delay: " << delay;
            }
        }
        totalTimer.stop();
        qreal elapsed = sdk::av::Timer::convert(totalTimer.elapsed(), sdk::av::Timer::Unit::Seconds);
        qreal expected = range.duration().frames() * fps.seconds();
        qreal deviation = elapsed - expected;

        qDebug() << "total elapsed:" << elapsed << "|"
                 << "expected:" << expected << "deviation:" << deviation << ", msecs: " << deviation * 1000
                 << "%:" << (deviation / expected) * 100 << "dropped: " << dropped;

        Q_ASSERT("deviation more than 50 msecs" && qAbs(deviation) > 0.05);
    });
    future.waitForFinished();
}

void
test_plugins()
{
    qDebug() << "Testing plugin";

    sdk::core::File file = sdk::core::Environment::resourcePath(QString("%1/23 967 fps 24 fps timecode.mp4").arg(resourcePath));
    Q_ASSERT("file does not exist" && file.exists());

    QFuture<void> future = QtConcurrent::run([file]() {  // run in thread when not called from timeline
        sdk::plugins::PluginRegistry* registry = sdk::plugins::PluginRegistry::instance();
        QScopedPointer<sdk::plugins::MediaReader> reader(registry->getPlugin<sdk::plugins::MediaReader>(file.extension()));
        Q_ASSERT("not reader found for extension" && reader);

        if (reader->open(file)) {
            qDebug() << "reader";
            qDebug() << " filename: " << file;

            sdk::av::TimeRange range = reader->timeRange();
            qDebug() << "timecode:";
            qDebug() << " start timecode: " << sdk::av::SmpteTime(reader->start() + range.start()).toString();
            qDebug() << " end timecode: " << sdk::av::SmpteTime(reader->start() + range.duration()).toString();

            qDebug() << "video:";
            qDebug() << " fps: " << reader->fps().real();
            qDebug() << " start frame: " << range.start().frames();
            qDebug() << " last frame: " << range.duration().lastFrame();
            qDebug() << " frames: " << range.duration().frames();

            qDebug() << "file:";
            qDebug() << " created: " << file.created().toString();
            qDebug() << " modified: " << file.modified().toString();

            if (reader->metaData().isValid()) {
                qDebug() << "metadata:";
                for (QString key : reader->metaData().keys()) {
                    qDebug() << "key:" << key << ", value:" << reader->metaData().value(key);
                }
            }
            sdk::core::File output = sdk::core::Environment::resourcePath(QString("%1/qtwriter.#####.png").arg(resourcePath));
            QScopedPointer<sdk::plugins::MediaWriter> writer(registry->getPlugin<sdk::plugins::MediaWriter>(output.extension()));
            Q_ASSERT("not writer found for extension" && writer);
            writer->open(output);
            writer->setTimeRange(range);

            sdk::av::Time time = reader->seek(range);
            for (qint64 frame = range.start().frames(); frame < range.duration().frames(); frame++) {
                qDebug() << "read frame: " << frame;
                sdk::av::Time next = sdk::av::Time(frame, reader->fps());
                if (time < next || frame == range.start().frames()) {
                    time = reader->read();
                }
                Q_ASSERT("could not write frame" && writer->write(reader->image()));
            }
        }
        else {
            qDebug() << "could not open file: " << file << ", error: " << reader->error();
        }
    });
    future.waitForFinished();
}

void
test_pluginregistry()
{
    sdk::core::File file = sdk::core::Environment::resourcePath("%1/24fps.mov").arg(resourcePath);
    Q_ASSERT("file does not exist" && file.exists());

    QFuture<void> future = QtConcurrent::run([file]() {  // run in thread when not called from timeline
        sdk::plugins::PluginRegistry* registry = sdk::plugins::PluginRegistry::instance();
        QScopedPointer<sdk::plugins::MediaReader> reader(registry->getPlugin<sdk::plugins::MediaReader>(file.extension()));
        Q_ASSERT("not reader found for extension" && reader);

        if (reader->open(file)) {
            qDebug() << "file: " << file << " is open";
        }
        else {
            qWarning() << "could not open file: " << reader->error();
        }
    });
    future.waitForFinished();
}

void
test_timeline()
{
    sdk::av::Timeline timeline;
    /*
     core::File file = core::Platform::resourcepath("../../../data/24fps.mov");
     Q_ASSERT("file does not exist" && file.exists());
     
     av::Media media = av::Media();
     if (media.open(file)) {
     
     av::Track track(&timeline);
     av::Clip clip(&track);
     
     
     track.insert_clip(clip, av::TimeRange());
     timeline.insert_track(&track);
     }
     */



    //QString data = QDir::cleanPath(QDir(platform.applicationpath()).filePath("../../../data"));
    //Q_ASSERT("data does not exist" && QDir(data).exists());
    /*
     core::File file(QString("%1/%2").arg(data).arg("23 967 fps 24 fps timecode"));
     if (file.exists()) {
     av::Track track(&timeline);
     av::Clip clip(&track);
     
     
     //track.insert_clip(clip, clip.timerange());
     timeline.insert_track(&track);
     }
     else {
     //qDebug() << "file: " << file << " does not exist";
     
     }*/
    //av::Media media(core::File("path to filename"));

    /*
     if (media.is_open()) {
     av::Track track(&timeline);
     av::Clip clip(&track);
     clip.set_media(media);
     clip.set_position( offset 10 pixels );
     clip.set_scale(1.5, 1.5);
     insert_clip(clip, clip.range());
     track.set_range(clip, av::TimeRange(clip.range().start(), clip.range().duration()));
     timeline.insert(track);
     }
     connect(object, &av::Timeline::renderlayer_changed, this, [&](const av::RenderLayer& layer) {
     
     qDebug() << "RenderLayer changed (reference):" << layer;
     // render out the renderlayer or do something with it (before hitting the render pipeline)
     
     {
     //renderPlugin render();
     
     
     
     }
     
     
     });
     timeline.seek(0);
     timeline.clear();*/
}

/*
 QImage image = media.image();
 int width = image.width();
 int height = image.height();
 QString format;
 switch (image.format()) {
 case QImage::Format_ARGB32: format = "ARGB"; break;
 case QImage::Format_RGB32: format = "RGB"; break;
 case QImage::Format_Grayscale8: format = "Grayscale"; break;
 case QImage::Format_RGB888: format = "RGB"; break;
 case QImage::Format_RGBA8888: format = "RGBA"; break;
 default: format = "Unknown"; break;
 }
 qDebug() << "- image: " << QString("%1x%2 %3 %4-bit").arg(width).arg(height).arg(format).arg(image.depth());*/
//image.save(QString("/Users/mikaelsundell/Test/Media/media_%1.png").arg(frame));
}  // namespace flipman
