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

#include <flipmansdk/core/file.h>
#include <flipmansdk/core/filerange.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/environment.h>

#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/plugins/qtwriter.h>
#include <flipmansdk/plugins/quicktimereader.h>

#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QtConcurrent>

namespace {
QString resourcepath = "../../data";
}

void
test_clip()
{
    qDebug() << "Testing clip";

    QString filename = "timecode export 29 97 04-05-03_10.mov";

    av::Clip clip;
    clip.set_color(Qt::red);
    clip.reset();

    av::Media media;
    core::File file = QString("%1/%2").arg(core::OS::resourcepath(resourcepath)).arg(filename);

    Q_ASSERT("file does not exist" && file.exists());
    media.open(file);
    Q_ASSERT("media has error" && !media.error().has_error());

    clip.set_position(0.5, 0.5);
    clip.set_scale(0.5, 0.5);
    qDebug() << clip.transform();
}


void
test_file()
{
    qDebug() << "Testing file";

    qDebug() << "Program path:" << core::OS::programpath();
    qDebug() << "Application path:" << core::OS::applicationpath();

    QString data = QDir::cleanPath(QDir(core::OS::applicationpath()).filePath(resourcepath));
    Q_ASSERT("data does not exist" && QDir(data).exists());

    QList<core::File> files = core::File::list(QString("%1/%2").arg(data).arg("frames"));

    for (core::File file : files) {
        qDebug() << file.display_name();
        core::FileRange range = file.filerange();
        if (range.is_valid()) {
            qDebug() << "- frames: " << range.size() << " start: " << range.start() << " end: " << range.end();
        }
    }

    Q_ASSERT(
        "file sequence does not match" && files[0].display_name() == "23.967.[86400-86445].exr"
        && files[1].display_name() == "23.967.00086446exr.exr" && files[2].display_name() == "23.967.[86447-86456].exr"
        && files[3].display_name() == "23.967.[86458-86512].exr" && files[4].display_name() == "23.967.00086513exr.exr"
        && files[5].display_name() == "23.967.[86514-86522].exr" && files[6].display_name() == "23.967.00086523exr.exr"
        && files[7].display_name() == "23.967.[86524-86643].exr" && files[8].display_name() == "23.967.00086644exr.exr"
        && files[9].display_name() == "23.967.[86645-86660].exr" && files[10].display_name() == "23.96700086457.exr"
        && files[11].display_name() == "abc.exr" && files[12].display_name() == "test.[86400-86401].exr"
        && files[13].display_name() == "test.00086583exr.exr" && files[14].display_name() == "test.[86584-86585].exr"
        && files[15].display_name() == "24fps.[86400-86496].jpg" && files[16].display_name() == "24fps.00086497jpg.jpg"
        && files[17].display_name() == "24fps.[86498-86519].jpg" && files[18].display_name() == "23.967.tif");
}

void
test_image()
{
    qDebug() << "Testing image";

    Q_ASSERT("format size half" && core::ImageFormat(core::ImageFormat::HALF).size() == 2);
    Q_ASSERT("format size float" && core::ImageFormat(core::ImageFormat::FLOAT).size() == 4);
    Q_ASSERT("format size double" && core::ImageFormat(core::ImageFormat::DOUBLE).size() == 8);

    QRect datawindow(0, 0, 1000, 1000);
    QRect displaywindow = datawindow;

    core::ImageFormat format(core::ImageFormat::UINT8);
    core::ImageBuffer buffer(datawindow, displaywindow, format, 3);

    quint8* data = buffer.data();
    size_t stride = buffer.stridesize();
    size_t pixelsize = buffer.pixelsize();

    QString testpath = core::OS::resourcepath(QString("%1/test").arg(resourcepath));
    Q_ASSERT("testpath does not exist" && QDir(testpath).exists());

    const int checker = 50;
    for (int y = 0; y < datawindow.height(); ++y) {
        for (int x = 0; x < datawindow.width(); ++x) {
            bool iswhite = ((x / checker) % 2) == ((y / checker) % 2);
            quint8 color = iswhite ? 255 : 0;
            quint8* pixel = data + (y * stride) + (x * pixelsize);
            for (size_t c = 0; c < buffer.channels(); ++c) {
                pixel[c] = color;
            }
        }
    }
    QImage checkerimage(reinterpret_cast<uchar*>(buffer.data()), datawindow.width(), datawindow.height(),
                        buffer.stridesize(), QImage::Format_RGB888);

    Q_ASSERT("could not save checker image" && checkerimage.save(QString("%1/%2").arg(testpath).arg("checker.png")));

    core::ImageBuffer ramp = buffer;
    ramp.detach();  // detach and alloc new
    data = ramp.data();
    for (int y = 0; y < datawindow.height(); ++y) {
        for (int x = 0; x < datawindow.width(); ++x) {
            quint8* pixel = data + (y * stride) + (x * pixelsize);
            float t = static_cast<float>(x) / datawindow.width();
            pixel[0] = static_cast<quint8>(255 * (1.0 - t));
            pixel[1] = static_cast<quint8>(255 * t);
            pixel[2] = static_cast<quint8>(128 + 127 * sin(3.14 * t));
        }
    }

    QImage rampimage(reinterpret_cast<uchar*>(ramp.data()), datawindow.width(), datawindow.height(),
                     buffer.stridesize(), QImage::Format_RGB888);

    Q_ASSERT("could not save ramp image" && rampimage.save(QString("%1/%2").arg(testpath).arg("ramp.png")));

    QImage checker2image(reinterpret_cast<uchar*>(buffer.data()), datawindow.width(), datawindow.height(),
                         buffer.stridesize(), QImage::Format_RGB888);

    Q_ASSERT("could not save checker2 image, should match checker image"
             && checker2image.save(QString("%1/%2").arg(testpath).arg("checker2.png")));

    core::ImageBuffer ramp16 = core::ImageBuffer::convert(ramp, core::ImageFormat::UINT16, 4);
    QImage ramp16image(reinterpret_cast<const uchar*>(ramp16.data()), datawindow.width(), datawindow.height(),
                       ramp16.stridesize(), QImage::Format_RGBX64);

    Q_ASSERT("could not save ramp16 image" && ramp16image.save(QString("%1/%2").arg(testpath).arg("ramp16.png")));
}

void
test_time()
{
    qDebug() << "Testing time";

    av::Time time;
    time.set_ticks(12000);
    time.set_timescale(24000);
    time.set_fps(av::Fps::fps_24());
    Q_ASSERT("ticks per frame" && time.tpf() == 1000);
    Q_ASSERT("ticks to frame" && time.frames() == 12);
    Q_ASSERT("frame to ticks" && time.ticks(12) == 12000);

    qDebug() << "ticks per frame: " << time.tpf();
    qDebug() << "ticks frames: " << time.frames();
    qDebug() << "frame to ticks: " << time.ticks(12);

    time.set_ticks(16016);
    time.set_timescale(30000);
    time.set_fps(av::Fps::fps_29_97());
    Q_ASSERT("ticks to frame" && time.frames() == 16);

    qDebug() << "ticks frames: " << time.frames();

    time = av::Time::convert(time, 24000);
    Q_ASSERT("ticks to frame" && time.frames() == 16);
    Q_ASSERT("ticks to frame" && time.frame(time.ticks()) == 16);
    Q_ASSERT("ticks align" && time.align(time.ticks()) == time.ticks());

    qDebug() << "ticks: " << time.ticks();
    qDebug() << "ticks frames: " << time.frames();

    time = av::Time::convert(time, 30000);
    qDebug() << "ticks: " << time.ticks();
    Q_ASSERT("ticks" && time.ticks() == 16016);

    time = av::Time(time.ticks() + time.ticks(1), time.timescale(), time.fps());
    Q_ASSERT("ticks align" && time.align(time.ticks()) == time.ticks());

    time.set_ticks(8677230);
    time.set_timescale(90000);
    time.set_fps(av::Fps::fps_23_976());

    qDebug() << "frames: " << time.frames();
    qDebug() << "ticks: " << time.ticks(time.frames() + 1);

    time.set_ticks(time.ticks(time.frames() + 1));
    qDebug() << "frames: " << time.frames();
}

void
test_timerange()
{
    av::TimeRange timerange;
    timerange.set_start(av::Time(12000, 24000, av::Fps::fps_24()));
    av::Time duration = av::Time::convert(av::Time(384000, 48000, av::Fps::fps_24()), timerange.start().timescale());
    timerange.set_duration(duration);
    Q_ASSERT("convert timescale" && duration.ticks() == 192000);
    Q_ASSERT("end ticks" && timerange.end().ticks() == 204000);

    qDebug() << "timerange: " << timerange.to_string();
}

void
test_fps()
{
    qDebug() << "Testing fps";

    Q_ASSERT("24 fps" && av::Fps::fps_24() == av::Fps(24, 1));
    qDebug() << "fps 24: " << av::Fps::fps_24().seconds();
    qDebug() << "fps 24: " << av::Fps(24, 1).seconds();
    qDebug() << "fps 24: " << 1 / 24.0;

    av::Time time;
    time.set_ticks(24000 * 100);  // typical ticks, 100 seconds
    time.set_timescale(24000);
    time.set_fps(av::Fps::fps_24());
    qint64 ticks;

    av::Fps guess23_97 = av::Fps::guess(23.976);
    Q_ASSERT("23.976 fps has drop frames" && guess23_97.drop_frame());
    qDebug() << "fps 23.976: " << guess23_97;

    av::Fps guess24 = av::Fps::guess(24);
    Q_ASSERT("24 fps is standard" && !guess24.drop_frame());
    qDebug() << "fps 24: " << guess24;

    av::Fps guess10 = av::Fps::guess(10);
    Q_ASSERT("10 fps is standard" && !guess10.drop_frame());
    qDebug() << "fps 10: " << guess10;

    av::Fps fps23_97 = av::Fps::fps_23_976();
    ticks = av::Time(time, fps23_97).ticks(1);
    Q_ASSERT("23.97 fps ticks" && ticks == 1001);
    qDebug() << "ticks 23_97: " << ticks;

    av::Fps fps24 = av::Fps::fps_24();
    ticks = av::Time(time, fps24).ticks(1);
    Q_ASSERT("24 fps ticks" && ticks == 1000);
    qDebug() << "ticks 24: " << ticks;

    av::Fps fps29_97 = av::Fps::fps_29_97();
    ticks = av::Time(time, fps29_97).ticks(2);
    qDebug() << "ticks 29_97: " << ticks;
    Q_ASSERT("29.97 fps ticks" && ticks == 1602);

    ticks = av::Time(time, fps29_97).ticks(5);
    qDebug() << "ticks 29_97: " << ticks;
    Q_ASSERT("29.97 fps ticks" && ticks == 4004);
}

void
test_smpte()
{
    qDebug() << "Testing SMPTE";

    av::Fps fps_24 = av::Fps::fps_24();
    qint64 frame = 86496;  // typical timecode, 01:00:04:00, 24 fps
    av::Time time(frame, fps_24);
    Q_ASSERT("86496 frames is 3604" && qFuzzyCompare(time.seconds(), 3604));
    qDebug() << "time: " << time.seconds();

    qint64 frame_fps = frame;
    av::SmpteTime smpte(av::Time(frame_fps, av::Fps::fps_24()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps_24(), av::Fps::fps_50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps_50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps_50(), av::Fps::fps_25());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps_25()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps_25(), av::Fps::fps_50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps_50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps_50(), av::Fps::fps_23_976());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps_23_976()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04.00");

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps_23_976(), av::Fps::fps_50());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps_50()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");

    frame_fps = av::SmpteTime::convert(frame_fps, av::Fps::fps_50(), av::Fps::fps_24());
    smpte = av::SmpteTime(av::Time(frame_fps, av::Fps::fps_24()));
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");

    qint64 frame_df_23_976 = av::SmpteTime::convert(frame, av::Fps::fps_23_976());
    qint64 frame_24 = av::SmpteTime::convert(frame_df_23_976, av::Fps::fps_23_976(),
                                             true);  // reverse
    Q_ASSERT("86496 dropframe is 86388" && frame_df_23_976 == 86388);
    Q_ASSERT("dropframe inverse does not match" && frame == frame_24);

    smpte = av::SmpteTime(time);
    Q_ASSERT("smpte is 01:00:04:00" && smpte.to_string() == "01:00:04:00");
    qDebug() << "smpte 24 fps: " << smpte.to_string();

    qint64 frame_30 = av::Fps::convert(frame, av::Fps::fps_24(), av::Fps::fps_30());
    smpte = av::SmpteTime(av::Time(frame_30, av::Fps::fps_30()));
    Q_ASSERT("smpte is 01:00:04:00 for 30 fps" && smpte.to_string() == "01:00:04:00");
    qDebug() << "smpte 30 fps: " << smpte.to_string();

    qint64 frame_23_976 = av::SmpteTime::convert(frame_24, av::Fps::fps_23_976());
    smpte = av::SmpteTime(av::Time(frame_23_976, av::Fps::fps_23_976()));
    Q_ASSERT("smpte is 01:00:04.00 for 23.976 fps" && smpte.to_string() == "01:00:04.00");
    qDebug() << "smpte 23.976 fps: " << smpte.to_string();

    qint64 frame_29_997 = 440658;
    smpte = av::SmpteTime(av::Time(frame_29_997, av::Fps::fps_29_97()));
    Q_ASSERT("smpte is 04:05:03.10" && smpte.to_string() == "04:05:03.10");
    qDebug() << "smpte 29_97 fps: " << smpte.to_string();

    frame_29_997 = 442698;
    smpte = av::SmpteTime(av::Time(frame_29_997, av::Fps::fps_29_97()));
    Q_ASSERT("smpte is 04:06:11.12" && smpte.to_string() == "04:06:11.12");
    qDebug() << "smpte 29_97 fps: " << smpte.to_string();

    // quicktime data:
    // 01:00:04:00
    // 2542
    // 01:46
    // 01:01:49:22 (uses floor rather than round, 22583)
    // 23.976 fps
    time = av::Time(2544542, 24000, av::Fps::fps_23_976());
    Q_ASSERT("time is 01:46" && time.to_string() == "01:46");
    Q_ASSERT("frames is 2542" && time.frames() == 2542);
    qDebug() << "time: " << time.to_string();
    qDebug() << "time frames: " << time.frames();

    frame = 2541;
    av::Time duration = av::Time(frame, av::Fps::fps_23_976());  // 0-2542, 2541 last frame
    Q_ASSERT("frames is 2541" && duration.frames() == frame);
    qDebug() << "time frames: " << duration.frames();

    frame = 86496;
    av::Time offset = av::Time(frame,
                               av::Fps::fps_24());  // typical timecode, 01:00:04:00, 24 fps
    qDebug() << "offset max: " << offset.frames();
    qDebug() << "offset smpte: " << av::SmpteTime(offset).to_string();

    frame = av::SmpteTime::convert(offset.frames(),
                                   av::Fps::fps_23_976());  // fix it to 01:00:04:00, match 23.976 timecode, drop frame
    Q_ASSERT("drop frame is 86388" && frame == 86388);
    qDebug() << "offset dropframe: " << frame;

    smpte = av::SmpteTime(av::Time(duration.frames() + frame, av::Fps::fps_23_976()));
    Q_ASSERT("smpte is 01:01:49.23" && smpte.to_string() == "01:01:49.23");
    qDebug() << "smpte: " << smpte.to_string();

    // ffmpeg data:
    // time_base=1/24000
    // duration_ts=187903716
    // 7829.344000
    // 24000/1001
    // 2:10:29.344000
    time = av::Time(187903716, 24000,
                    av::Fps::fps_24());  // typical duration, 02:10:29.344000, 24 fps
    Q_ASSERT("seconds 7829.32" && qFuzzyCompare(time.seconds(), 7829.3215));
    qDebug() << "seconds: " << time.seconds();

    smpte = av::SmpteTime(time);
    Q_ASSERT("smpte is 02:10:29:08 for 24 fps"
             && smpte.to_string() == "02:10:29:08");  // *:08 equals 8/24 of a second = .344000
    qDebug() << "smpte 24 fps: " << smpte.to_string();

    // resolve data:
    // frame: 87040, converted to 87148 at fps: 23.967
    // 01:00:31:04
    // 01:00:30 (wall clock time)
    // 24 NDF is used in Resolve for 23.967 for timecode
    frame = 87040;
    time = av::Time(frame, av::Fps::fps_23_976());
    Q_ASSERT("time is 01:00:30" && time.to_string() == "01:00:30");
    qDebug() << "time: " << time.to_string();

    smpte = av::SmpteTime(time);
    Q_ASSERT("smpte is 01:00:31.04 for 23.976 fps" && smpte.to_string() == "01:00:31.04");
    qDebug() << "smpte 23.976: " << smpte.to_string();
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
            core::File file = QString("%1/%2").arg(core::OS::resourcepath(resourcepath)).arg(filename);
            Q_ASSERT("file does not exist" && file.exists());
            av::Media media;
            if (media.open(file)) {
                core::File output = QString("%1/test/%2%3").arg(file.dirname()).arg(file.basename()).arg(".#####.png");
                av::MediaProcessor processor;

                QObject::connect(&processor, &av::MediaProcessor::progress_changed,
                                 [&file](const av::Time& time, const av::TimeRange& range) {
                                     qDebug() << "processing:" << file << " - time:" << time.to_string()
                                              << ", frame: " << time.frames() << ", timerange: " << range.to_string();
                                 });
                av::TimeRange timerange = media.timerange();
                if (processor.write(media, timerange, output)) {
                    qDebug() << "processed file: " << output << ", timerange: " << timerange.to_string();
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
        av::Fps fps = av::Fps::fps_23_976();
        qint64 start = 1, duration = 24 * 400;
        av::TimeRange range(av::Time(start, fps), av::Time(duration, fps));
        av::Timer totaltimer;
        totaltimer.start();
        av::Timer timer;
        timer.start(fps);

        qDebug() << "range: start:" << range.start().frames() << ", duration: " << range.duration().frames();
        qint64 frames = range.duration().frames();

        qint64 dropped = 0;
        for (qint64 frame = range.start().frames(); frame < range.duration().frames(); frame++) {
            quint64 currenttime = timer.elapsed();
            quint64 delay = QRandomGenerator::global()->bounded(1, 80);
            timer.sleep(delay);
            timer.wait();

            qreal elapsed = av::Timer::convert(timer.elapsed() - currenttime, av::Timer::Unit::Seconds);
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
        totaltimer.stop();
        qreal elapsed = av::Timer::convert(totaltimer.elapsed(), av::Timer::Unit::Seconds);
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

    core::File file = core::OS::resourcepath(QString("%1/23 967 fps 24 fps timecode.mp4").arg(resourcepath));
    Q_ASSERT("file does not exist" && file.exists());

    QFuture<void> future = QtConcurrent::run([file]() {  // run in thread when not called from timeline
        plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
        QScopedPointer<plugins::MediaReader> reader(registry->get_plugin<plugins::MediaReader>(file.extension()));
        Q_ASSERT("not reader found for extension" && reader);

        if (reader->open(file)) {
            qDebug() << "reader";
            qDebug() << " filename: " << file;

            av::TimeRange range = reader->timerange();
            qDebug() << "timecode:";
            qDebug() << " start timecode: " << av::SmpteTime(reader->start() + range.start()).to_string();
            qDebug() << " end timecode: " << av::SmpteTime(reader->start() + range.duration()).to_string();

            qDebug() << "video:";
            qDebug() << " fps: " << reader->fps().real();
            qDebug() << " start frame: " << range.start().frames();
            qDebug() << " last frame: " << range.duration().lastframe();
            qDebug() << " frames: " << range.duration().frames();

            qDebug() << "file:";
            qDebug() << " created: " << file.created().toString();
            qDebug() << " modified: " << file.modified().toString();

            if (reader->metadata().is_valid()) {
                qDebug() << "metadata:";
                for (QString key : reader->metadata().keys()) {
                    qDebug() << "key:" << key << ", value:" << reader->metadata().value(key);
                }
            }
            core::File output = core::OS::resourcepath(QString("%1/qtwriter.#####.png").arg(resourcepath));
            QScopedPointer<plugins::MediaWriter> writer(registry->get_plugin<plugins::MediaWriter>(output.extension()));
            Q_ASSERT("not writer found for extension" && writer);
            writer->open(output);
            writer->set_timerange(range);

            av::Time time = reader->seek(range);
            for (qint64 frame = range.start().frames(); frame < range.duration().frames(); frame++) {
                qDebug() << "read frame: " << frame;
                av::Time next = av::Time(frame, reader->fps());
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
    core::File file = core::OS::resourcepath("%1/24fps.mov").arg(resourcepath);
    Q_ASSERT("file does not exist" && file.exists());

    QFuture<void> future = QtConcurrent::run([file]() {  // run in thread when not called from timeline
        plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
        QScopedPointer<plugins::MediaReader> reader(registry->get_plugin<plugins::MediaReader>(file.extension()));
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
    av::Timeline timeline;
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
