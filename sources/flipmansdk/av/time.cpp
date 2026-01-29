// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QDebug>
#include <av/time.h>

namespace flipman::sdk::av {
class TimePrivate : public QSharedData {
public:
    qreal tpf();
    qint64 ticks(qint64 frame);
    qint64 frame(qint64 ticks);
    qint64 frames();
    QString toString(qreal seconds);
    struct Data {
        Fps fps = Fps::fps24();
        qint64 ticks = 0;
        qint32 timeScale = 24000;  // default timescale
    };
    Data d;
};

qreal
TimePrivate::tpf()
{
    return static_cast<qreal>(d.timeScale) / d.fps.real();
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
TimePrivate::toString(qreal seconds)
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

Time::Time(qint64 ticks, qint32 timeScale, const Fps& fps)
    : p(new TimePrivate())
{
    p->d.ticks = ticks;
    p->d.timeScale = timeScale;
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
    p->d.ticks = p->d.timeScale * seconds;
}

Time::Time(const Time& other, qint64 ticks)
    : p(other.p)
{
    setTicks(ticks);
}

Time::Time(const Time& other, const Fps& fps)
    : p(other.p)
{
    setFps(fps);
}

Time::Time(const Time& other)
    : p(other.p)
{}

Time::~Time() {}


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
Time::timeScale() const
{
    return p->d.timeScale;
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
Time::lastFrame() const
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
    return static_cast<qreal>(p->d.ticks) / p->d.timeScale;
}

QString
Time::toString(qint64 ticks) const
{
    return p->toString(static_cast<qreal>(ticks) / p->d.timeScale);
}

QString
Time::toString() const
{
    return p->toString(seconds());
}

bool
Time::isValid() const
{
    return p->d.timeScale > 0;
}

void
Time::reset()
{
    p.reset(new TimePrivate());
}

void
Time::setTicks(qint64 ticks)
{
    if (p->d.ticks != ticks) {
        p.detach();
        p->d.ticks = ticks;
    }
}

void
Time::setTimeScale(qint32 timeScale)
{
    if (p->d.timeScale != timeScale) {
        p.detach();
        p->d.timeScale = timeScale;
    }
}

void
Time::setFps(const Fps& fps)
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
    return p->d.ticks == other.p->d.ticks && p->d.timeScale == other.p->d.timeScale && p->d.fps == other.p->d.fps;
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
    Q_ASSERT("timescale does not match" && p->d.timeScale == other.p->d.timeScale);
    return Time(p->d.ticks + other.p->d.ticks, p->d.timeScale, p->d.fps);
}

Time
Time::operator-(const Time& other) const
{
    Q_ASSERT("timescale does not match" && p->d.timeScale == other.p->d.timeScale);
    return Time(p->d.ticks - other.p->d.ticks, p->d.timeScale, p->d.fps);
}

Time::operator double() const { return seconds(); }

Time
Time::convert(const Time& time, const Fps& to)
{
    return convert(time, to.frameScale());
}

Time
Time::convert(const Time& time, qint32 timeScale)
{
    qint64 numerator = time.ticks() * timeScale;
    qint64 remainder = numerator % time.timeScale();
    qint64 ticks = numerator / time.timeScale();
    if (remainder >= (time.timeScale() / 2)) {
        if (time.ticks() > 0) {
            ticks += 1;
        }
        else if (time.ticks() < 0) {
            ticks -= 1;
        }
    }
    return Time(ticks, timeScale, time.fps());
}
}  // namespace flipman::sdk::av
