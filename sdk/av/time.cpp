// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QDebug>
#include <av/time.h>

namespace av {
class TimePrivate : public QSharedData {
public:
    qreal tpf();
    qint64 ticks(qint64 frame);
    qint64 frame(qint64 ticks);
    qint64 frames();
    QString to_string(qreal seconds);
    struct Data {
        Fps fps = Fps::fps_24();
        qint64 ticks = 0;
        qint32 timescale = 24000;  // default timescale
    };
    Data d;
};

qreal
TimePrivate::tpf()
{
    return static_cast<qreal>(d.timescale) / d.fps.real();
};

qint64
TimePrivate::ticks(qint64 frame)
{
    return static_cast<qint64>(std::round(frame * tpf()));
}

qint64
TimePrivate::frame(qint64 ticks)
{
    return static_cast<qint64>(std::round(ticks / tpf()));
}

qint64
TimePrivate::frames()
{
    return static_cast<qint64>(std::round(d.ticks / tpf()));
}

QString
TimePrivate::to_string(qreal seconds)
{
    qint64 secs = qFloor(seconds);  // complete seconds
    qint64 minutes = secs / 60;
    qint64 hours = minutes / 60;
    secs %= 60;
    minutes %= 60;
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
    else {
        return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    }
}

Time::Time()
    : p(new TimePrivate())
{}

Time::Time(qint64 ticks, qint32 timescale, const Fps& fps)
    : p(new TimePrivate())
{
    p->d.ticks = ticks;
    p->d.timescale = timescale;
    p->d.fps = fps;
}

Time::Time(qint64 frame, const Fps& fps)
    : p(new TimePrivate())
{
    p->d.fps = fps;
    p->d.ticks = ticks(frame);
}

Time::Time(qreal seconds, const Fps& fps)
    : p(new TimePrivate())
{
    p->d.fps = fps;
    p->d.ticks = p->d.timescale * seconds;
}

Time::Time(const Time& other, qint64 ticks)
    : p(other.p)
{
    set_ticks(ticks);
}

Time::Time(const Time& other, const Fps& fps)
    : p(other.p)
{
    set_fps(fps);
}

Time::Time(const Time& other)
    : p(other.p)
{}

Time::~Time() {}

bool
Time::is_valid() const
{
    return p->d.timescale > 0;
}

Fps
Time::fps() const
{
    return p->d.fps;
}

qint64
Time::ticks() const
{
    return p->d.ticks;
}

qint64
Time::ticks(qint64 frame) const
{
    return p->ticks(frame);
}

qint32
Time::timescale() const
{
    return p->d.timescale;
}

qint64
Time::tpf() const
{
    return static_cast<qint64>(std::round(p->tpf()));
}

qint64
Time::frame(qint64 ticks) const
{
    return p->frame(ticks);
}

qint64
Time::lastframe() const
{
    return p->frames() - 1;
}

qint64
Time::frames() const
{
    return p->frames();
}

qint64
Time::align(qint64 ticks) const
{
    return this->ticks(p->frame(ticks));
}

qreal
Time::seconds() const
{
    return static_cast<qreal>(p->d.ticks) / p->d.timescale;
}

QString
Time::to_string(qint64 ticks) const
{
    return p->to_string(static_cast<qreal>(ticks) / p->d.timescale);
}

QString
Time::to_string() const
{
    return p->to_string(seconds());
}

void
Time::reset()
{
    p.reset(new TimePrivate());
}

void
Time::set_ticks(qint64 ticks)
{
    if (p->d.ticks != ticks) {
        p.detach();
        p->d.ticks = ticks;
    }
}

void
Time::set_timescale(qint32 timescale)
{
    if (p->d.timescale != timescale) {
        p.detach();
        p->d.timescale = timescale;
    }
}

void
Time::set_fps(const Fps& fps)
{
    if (p->d.fps != fps) {
        p.detach();
        p->d.fps = fps;
    }
}

Time&
Time::operator=(const Time& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
Time::operator==(const Time& other) const
{
    return p->d.ticks == other.p->d.ticks && p->d.timescale == other.p->d.timescale && p->d.fps == other.p->d.fps;
}

bool
Time::operator!=(const Time& other) const
{
    return !(*this == other);
}

bool
Time::operator<(const Time& other) const
{
    return seconds() < other.seconds();
}

bool
Time::operator>(const Time& other) const
{
    return seconds() > other.seconds();
}

bool
Time::operator<=(const Time& other) const
{
    return seconds() <= other.seconds();
}

bool
Time::operator>=(const Time& other) const
{
    return seconds() >= other.seconds();
}

Time
Time::operator+(const Time& other) const
{
    Q_ASSERT("timescale does not match" && p->d.timescale == other.p->d.timescale);
    return Time(p->d.ticks + other.p->d.ticks, p->d.timescale, p->d.fps);
}

Time
Time::operator-(const Time& other) const
{
    Q_ASSERT("timescale does not match" && p->d.timescale == other.p->d.timescale);
    return Time(p->d.ticks - other.p->d.ticks, p->d.timescale, p->d.fps);
}

Time::operator double() const { return seconds(); }

Time
Time::convert(const Time& time, const Fps& to)
{
    return convert(time, to.framescale());
}

Time
Time::convert(const Time& time, qint32 timescale)
{
    qint64 numerator = time.ticks() * timescale;
    qint64 remainder = numerator % time.timescale();
    qint64 ticks = numerator / time.timescale();
    if (remainder >= (time.timescale() / 2)) {
        if (time.ticks() > 0) {
            ticks += 1;
        }
        else if (time.ticks() < 0) {
            ticks -= 1;
        }
    }
    return Time(ticks, timescale, time.fps());
}
}  // namespace av
