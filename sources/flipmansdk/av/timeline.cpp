// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.


#include <flipmansdk/av/media.h>
#include <flipmansdk/av/timeline.h>
#include <flipmansdk/av/timer.h>
#include <flipmansdk/av/track.h>

#include <QPointer>
#include <QThreadPool>
#include <QtGlobal>

#include <QDebug>

namespace flipman::sdk::av {
class TimelinePrivate {
public:
    void init();
    void reset();
    struct Data {
        Time time = Time();
        TimeRange timeRange = TimeRange();
        TimeRange ioRange = TimeRange();
        Fps fps = Fps();
        int width = 1920;
        int height = 1080;
        QList<Track*> tracks;
        QThreadPool threadPool;
        std::atomic<bool> loop = false;
        std::atomic<bool> everyFrame = false;
        std::atomic<bool> playing = false;
        core::Error error;
    };
    Data d;
    QPointer<Timeline> object;
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
    p->object = this;
}

Timeline::~Timeline() {}

void
Timeline::reset()
{
    p.reset(new TimelinePrivate());
    p->init();
}

bool
Timeline::isPlaying() const
{
    return true;
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
    return Time();
}

SmpteTime
Timeline::timeCode() const
{
    return SmpteTime();
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
Timeline::hasTrack(Track* track)
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
    p->d.threadPool.maxThreadCount();
}

core::Error
Timeline::error() const
{
    return p->d.error;
}

void
Timeline::insertTrack(Track* track)
{
    p->d.tracks.append(track);
}

void
Timeline::removeTrack(Track* track)
{
    Q_ASSERT("does not contain track" && hasTrack(track));
    p->d.tracks.remove(p->d.tracks.indexOf(track));
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
{}

void
Timeline::play()
{}

void
Timeline::stop()
{
    p->d.playing = false;
}
}  // namespace flipman::sdk::av
