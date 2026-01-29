// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/av/timer.h>
#include <flipmansdk/av/fps.h>

#include <mach/mach.h>
#include <mach/mach_time.h>

#include <QPointer>
#include <QDebug>
#include <QThread>
#include <QtConcurrent>

namespace flipman::sdk::av {
class TimerPrivate
{
    public:
        TimerPrivate();
        quint64 nano(const av::Fps& fps);
        quint64 nano(quint64 ticks);
        quint64 ticks(quint64 time);
        quint64 elapsed();
        struct Data
        {
            quint64 start = 0;
            quint64 stop = 0;
            quint64 timer = 0;
            quint64 next = 0;
            QList<quint64> laps;
            mach_timebase_info_data_t timeBase;
        };
        Data d;
};

TimerPrivate::TimerPrivate()
{
    mach_timebase_info(&d.timeBase);
}

quint64
TimerPrivate::nano(const av::Fps& fps) {
    return static_cast<quint64>((1e9 * fps.denominator()) / fps.numerator());
}

quint64
TimerPrivate::nano(quint64 ticks) {
    return (ticks * d.timeBase.numer) / d.timeBase.denom;
}

quint64
TimerPrivate::ticks(quint64 time) {
    return (time * d.timeBase.denom) / d.timeBase.numer;
}

quint64
TimerPrivate::elapsed() {
    quint64 end = d.stop > 0 ? d.stop : mach_absolute_time();
    return (end - d.start) * d.timeBase.numer / d.timeBase.denom;
}

Timer::Timer()
: p(new TimerPrivate())
{
}

Timer::~Timer()
{
}

bool
Timer::isValid() const
{
    return p->d.start > 0;
}

void
av::Timer::start()
{
    p->d.start = mach_absolute_time();
    p->d.stop = 0;
}

void
Timer::start(const av::Fps& fps)
{
    Q_ASSERT("fps is zero" && fps.seconds() > 0);
    
    p->d.timer = mach_absolute_time();
    p->d.next = p->d.timer + p->ticks(p->nano(fps));
    p->d.stop = 0;
}

void
Timer::stop()
{
    p->d.stop = mach_absolute_time();
    p->d.next = p->d.stop;
}

void
Timer::restart()
{
    p->d.laps.clear();
    start();
}

void
Timer::lap()
{
    if (!p->d.laps.isEmpty()) {
        p->d.laps.append(elapsed() - p->d.laps.last());
    }
    else {
        p->d.laps.append(elapsed());
    }
}

bool
Timer::next(const av::Fps& fps)
{
    quint64 currenttime = mach_absolute_time();
    p->d.next += p->ticks(p->nano(fps));
    return p->d.next > currenttime;
}

void 
Timer::wait()
{
    Q_ASSERT("start time must be less than nexttime" && p->d.start < p->d.next);
    
    mach_wait_until(p->d.next);
}

void
Timer::sleep(quint64 msecs)
{
    quint64 sleepTime = static_cast<quint64>(msecs * 1e6);
    quint64 currentTime = mach_absolute_time();
    quint64 targetTime = currentTime + p->ticks(sleepTime);
    
    Q_ASSERT("sleep time already passed" && currentTime < targetTime);
    mach_wait_until(targetTime);
}

quint64
Timer::elapsed() const
{
    quint64 end = p->d.stop > 0 ? p->d.stop : mach_absolute_time();
    return p->nano(end - p->d.start);
}

QList<quint64>
Timer::laps() const
{
    return p->d.laps;
}

void
Timer::reset()
{
    p.reset(new TimerPrivate());
}

qreal
Timer::convert(quint64 nano, Unit unit)
{
    switch (unit) {
    case Unit::Seconds:
        return static_cast<qreal>(nano) / 1e9;
        break;
    case Unit::Minutes:
        return static_cast<qreal>(nano) / (60 * 1e9);
        break;
    case Unit::Hours:
        return static_cast<qreal>(nano) / (3600 * 1e9);
        break;
    default:
        return static_cast<qreal>(nano);
    }
}
}
