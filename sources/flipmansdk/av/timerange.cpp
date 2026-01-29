// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QtGlobal>
#include <flipmansdk/av/timerange.h>

namespace flipman::sdk::av {
class TimeRangePrivate : public QSharedData {
public:
    struct Data {
        Time start;
        Time duration;
    };
    Data d;
};

TimeRange::TimeRange()
    : p(new TimeRangePrivate())
{}

TimeRange::TimeRange(Time start, Time duration)
    : p(new TimeRangePrivate())
{
    Q_ASSERT(start.timeScale() == duration.timeScale());
    p->d.start = start;
    p->d.duration = duration;
}

TimeRange::TimeRange(const TimeRange& other)
    : p(other.p)
{}

TimeRange::~TimeRange() {}

bool
TimeRange::isValid() const
{
    return p->d.start.isValid() && p->d.duration.isValid() && p->d.duration.ticks() > 0;
}

Time
TimeRange::start() const
{
    return p->d.start;
}

Time
TimeRange::duration() const
{
    return p->d.duration;
}

Time
TimeRange::end() const
{
    return p->d.start + p->d.duration;
}

Time
TimeRange::bound(const Time& time)
{
    Q_ASSERT(time.timeScale() == start().timeScale());
    return Time(qBound(start().ticks(), time.ticks(), end().ticks()), time.timeScale(), time.fps());
}

Time
TimeRange::bound(const Time& time, bool loop)
{
    Q_ASSERT(time.timeScale() == start().timeScale());
    qint64 tpf = time.tpf();
    qint64 lower = start().ticks();
    qint64 upper = end().ticks() - tpf;
    if (loop) {
        qint64 range = upper - lower + tpf;
        qint64 wrapped = lower + ((time.ticks() - lower) % range + range) % range;
        return Time(wrapped, time.timeScale(), time.fps());
    }
    else {
        return Time(qBound(lower, time.ticks(), upper), time.timeScale(), time.fps());
    }
}

bool
TimeRange::intersects(const TimeRange& other) const
{
    return (p->d.start < other.end()) && (other.start() < end());
}

QString
TimeRange::toString() const
{
    return QString("%1 / %2").arg(p->d.start.toString()).arg(p->d.duration.toString());
}

void
TimeRange::reset()
{
    p.reset(new TimeRangePrivate());
}

void
TimeRange::setStart(Time start)
{
    p.detach();
    p->d.start = start;
}

void
TimeRange::setDuration(Time duration)
{
    p.detach();
    p->d.duration = duration;
}

TimeRange&
TimeRange::operator=(const TimeRange& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
TimeRange::operator==(const TimeRange& other) const
{
    return p->d.start == other.p->d.start && p->d.duration == other.p->d.duration;
}

bool
TimeRange::operator!=(const TimeRange& other) const
{
    return !(*this == other);
}

TimeRange
TimeRange::convert(const TimeRange& timerange, const Fps& to)
{
    return TimeRange(Time::convert(timerange.start(), to), Time::convert(timerange.duration(), to));
}

TimeRange
TimeRange::convert(const TimeRange& timerange, qint32 timescale)
{
    return TimeRange(Time::convert(timerange.start(), timescale), Time::convert(timerange.duration(), timescale));
}
}  // namespace flipman::sdk::av
