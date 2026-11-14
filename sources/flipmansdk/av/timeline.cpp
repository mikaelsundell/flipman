// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include <flipmansdk/av/media.h>
#include <flipmansdk/av/timeline.h>
#include <flipmansdk/av/timer.h>
#include <flipmansdk/av/track.h>
#include <QPointer>
#include <QThreadPool>
#include <QtGlobal>

namespace flipman::sdk::av {

class TimelinePrivate {
public:
    void init();
    void reset();

    struct Data {
        Time time = Time();
        TimeRange timeRange = TimeRange();
        TimeRange ioRange = TimeRange();
        Fps fps = Fps::fps24();
        Time sourceStart = Time::zero(Fps::fps24());
        core::Error error;
        QList<Track*> tracks;
        QThreadPool threadPool;
        std::atomic<bool> loop = false;
        std::atomic<bool> everyFrame = false;
        std::atomic<bool> playing = false;
        int width = 1920;
        int height = 1080;
        QPointer<Timeline> object;
    };
    Data d;
};

void
TimelinePrivate::init()
{
    d.threadPool.setMaxThreadCount(1);
}

Timeline::Timeline(QObject* parent)
    : QObject(parent)
    , p(new TimelinePrivate())
{
    p->init();
    p->d.object = this;
}

Timeline::~Timeline() {}

void
Timeline::reset()
{
    p.reset(new TimelinePrivate());
    p->init();
    p->d.object = this;
    Q_EMIT playChanged(false);
}

bool
Timeline::isPlaying() const
{
    return p->d.playing;
}

TimeRange
Timeline::timeRange() const
{
    return p->d.timeRange;
}

TimeRange
Timeline::ioRange() const
{
    return p->d.ioRange;
}

Time
Timeline::time() const
{
    return p->d.time;
}

Time
Timeline::startTime() const
{
    return p->d.sourceStart;
}

SmpteTime
Timeline::timeCode() const
{
    return SmpteTime(Time::fromFrames(p->d.sourceStart.frames() + p->d.time.frames(), p->d.fps));
}

Fps
Timeline::fps() const
{
    return p->d.fps;
}

int
Timeline::width() const
{
    return p->d.width;
}

int
Timeline::height() const
{
    return p->d.height;
}

bool
Timeline::loop() const
{
    return p->d.loop;
}

bool
Timeline::hasTrack(Track* track) const
{
    return p->d.tracks.contains(track);
}

QList<Track*>
Timeline::tracks() const
{
    return p->d.tracks;
}

int
Timeline::threadCount() const
{
    return p->d.threadPool.maxThreadCount();
}

core::Error
Timeline::error() const
{
    return p->d.error;
}

void
Timeline::insertTrack(Track* track)
{
    if (!track || hasTrack(track))
        return;

    p->d.tracks.append(track);
    connect(track, &QObject::destroyed, this, [this, track]() { p->d.tracks.removeAll(track); });
}

void
Timeline::removeTrack(Track* track)
{
    p->d.tracks.removeAll(track);
}

void
Timeline::setLoop(bool loop)
{
    if (p->d.loop != loop) {
        p->d.loop = loop;
        Q_EMIT loopChanged(loop);
    }
}

void
Timeline::setTimeRange(const TimeRange& timeRange)
{
    if (p->d.timeRange != timeRange) {
        p->d.timeRange = timeRange;
        if (timeRange.isValid()) {
            p->d.fps = timeRange.duration().fps();
            p->d.time = timeRange.start();
        }
        Q_EMIT timerangeChanged(timeRange);
    }
}

void
Timeline::setIoRange(const TimeRange& ioRange)
{
    if (p->d.ioRange != ioRange) {
        p->d.ioRange = ioRange;
        Q_EMIT ioRangeChanged(ioRange);
    }
}

void
Timeline::setEveryFrame(bool everyFrame)
{
    if (p->d.everyFrame != everyFrame) {
        p->d.everyFrame = everyFrame;
        Q_EMIT everyFrameChanged(everyFrame);
    }
}

void
Timeline::setWidth(int width)
{
    if (p->d.width != width) {
        p->d.width = width;
        Q_EMIT widthChanged(width);
    }
}

void
Timeline::setHeight(int height)
{
    if (p->d.height != height) {
        p->d.height = height;
        Q_EMIT heightChanged(height);
    }
}

void
Timeline::setThreadCount(int threadCount)
{
    if (p->d.threadPool.maxThreadCount() != threadCount) {
        p->d.threadPool.setMaxThreadCount(threadCount);
    }
}

void
Timeline::seek(const Time& time)
{
    if (!time.isValid() || !p->d.timeRange.isValid())
        return;

    const auto next = p->d.timeRange.bound(Time::fromFrames(time.frames(), p->d.fps), false);
    if (next != p->d.time) {
        p->d.time = next;
        Q_EMIT timeChanged(next);
        Q_EMIT timeCodeChanged(Time::fromFrames(p->d.sourceStart.frames() + next.frames(), p->d.fps));
    }
}

void
Timeline::play()
{
    if (!p->d.playing && p->d.timeRange.isValid()) {
        p->d.playing = true;
        Q_EMIT playChanged(true);
    }
}

void
Timeline::stop()
{
    if (p->d.playing) {
        p->d.playing = false;
        Q_EMIT playChanged(false);
    }
}

void
Timeline::setStartTime(const Time& time)
{
    if (time.isValid())
        p->d.sourceStart = Time::fromSeconds(time.seconds(), p->d.fps);
}

void
Timeline::setFps(const Fps& fps)
{
    if (!fps.isValid() || fps == p->d.fps)
        return;

    const auto frame = p->d.time.frames();
    const auto sourceSeconds = p->d.sourceStart.seconds();
    if (p->d.timeRange.isValid())
        p->d.timeRange = TimeRange(Time::fromFrames(p->d.timeRange.start().frames(), fps),
                                   Time::fromFrames(p->d.timeRange.duration().frames(), fps));
    if (p->d.ioRange.isValid())
        p->d.ioRange = TimeRange(Time::fromFrames(p->d.ioRange.start().frames(), fps),
                                 Time::fromFrames(p->d.ioRange.duration().frames(), fps));
    p->d.fps = fps;
    p->d.sourceStart = Time::fromSeconds(sourceSeconds, fps);
    p->d.time = Time::fromFrames(frame, fps);
    Q_EMIT fpsChanged(fps);
    Q_EMIT timerangeChanged(p->d.timeRange);
    Q_EMIT ioRangeChanged(p->d.ioRange);
    Q_EMIT timeChanged(p->d.time);
}

}  // namespace flipman::sdk::av
