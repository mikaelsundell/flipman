// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include <QPointer>
#include <QThreadPool>
#include <QtGlobal>
#include <av/media.h>
#include <av/timeline.h>
#include <av/timer.h>
#include <av/track.h>

#include <QDebug>

namespace av {
class TimelinePrivate {
public:
    void init();
    void reset();
    struct Data {
        Time time = Time();
        TimeRange timerange = TimeRange();
        TimeRange iorange = TimeRange();
        Fps fps = Fps();
        int width = 1920;
        int height = 1080;
        QList<Track*> tracks;
        QThreadPool threadpool;
        std::atomic<bool> loop = false;
        std::atomic<bool> everyframe = false;
        std::atomic<bool> playing = false;
        core::Error error;
    };
    Data d;
    QPointer<Timeline> object;
};

void
TimelinePrivate::init()
{
    d.threadpool.setMaxThreadCount(1);
}

Timeline::Timeline(QObject* parent)
    : core::Container(parent)
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
Timeline::is_playing() const
{
    return true;
}

TimeRange
Timeline::timerange() const
{
    return p->d.timerange;
}

TimeRange
Timeline::io() const
{
    return p->d.iorange;
}

Time
Timeline::time() const
{
    return p->d.time;
}

Time
Timeline::starttime() const
{
    return Time();
}

SmpteTime
Timeline::timecode() const
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
Timeline::has_track(Track* track)
{
    return p->d.tracks.contains(track);
}

QList<Track*>
Timeline::tracks() const
{
    return p->d.tracks;
}

int
Timeline::threadcount() const
{
    p->d.threadpool.maxThreadCount();
}

core::Error
Timeline::error() const
{
    return p->d.error;
}

void
Timeline::insert_track(Track* track)
{
    p->d.tracks.append(track);
}

void
Timeline::remove_track(Track* track)
{
    Q_ASSERT("does not contain track" && has_track(track));
    p->d.tracks.remove(p->d.tracks.indexOf(track));
}

void
Timeline::set_loop(bool loop)
{
    if (p->d.loop != loop) {
        p->d.loop = loop;
        loop_changed(loop);
    }
}

void
Timeline::set_timerange(const TimeRange& timerange)
{
    if (p->d.timerange != timerange) {
        p->d.timerange = timerange;
        timerange_changed(timerange);
    }
}

void
Timeline::set_io(const TimeRange& io)
{
    if (p->d.iorange != io) {
        p->d.iorange = io;
        io_changed(io);
    }
}

void
Timeline::set_everyframe(bool everyframe)
{
    if (p->d.everyframe != everyframe) {
        p->d.everyframe = everyframe;
        everyframe_changed(everyframe);
    }
}

void
Timeline::set_width(int width)
{
    if (p->d.width != width) {
        p->d.width = width;
        width_changed(width);
    }
}

void
Timeline::set_height(int height)
{
    if (p->d.height != height) {
        p->d.height = height;
        height_changed(height);
    }
}

void
Timeline::set_threadcount(int threadcount)
{
    if (p->d.threadpool.maxThreadCount() != threadcount) {
        p->d.threadpool.setMaxThreadCount(threadcount);
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
}  // namespace av
