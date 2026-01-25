// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include <av/smptetime.h>

#include <QDebug>

namespace av {
class SmpteTimePrivate : public QSharedData {
public:
    qint64 frame() const;
    void update();
    struct Data {
        Time time;
        quint32 counter = 0;
        qint16 hours = 0;
        qint16 minutes = 0;
        qint16 seconds = 0;
        qint16 frames = 0;
        qint16 subframes = 1;
        qint16 subframe_divisor = 0;
        bool negatives = true;
        bool fullhours = true;
    };
    Data d;
};

qint64
SmpteTimePrivate::frame() const
{
    Q_ASSERT("time is not valid" && d.time.is_valid());

    return d.time.frames();
}

void
SmpteTimePrivate::update()
{
    Q_ASSERT("time is not valid" && d.time.is_valid());

    qint64 frame = d.time.frames();
    qint16 framequanta = d.time.fps().framequanta();
    bool is_negative = false;
    if (frame < 0) {
        is_negative = true;
        frame = -frame;
    }
    frame = SmpteTime::convert(frame, d.time.fps(), true);
    d.frames = frame % framequanta;
    frame /= framequanta;
    d.seconds = frame % 60;
    frame /= 60;
    d.minutes = frame % 60;
    frame /= 60;
    if (d.fullhours) {
        frame %= 24;
        if (is_negative && !d.negatives) {
            is_negative = false;
            frame = 23 - frame;
        }
    }
    d.hours = frame;
    if (is_negative) {
        d.minutes |= 0x80;  // indicate negative number
    }
}

SmpteTime::SmpteTime()
    : p(new SmpteTimePrivate())
{}

SmpteTime::SmpteTime(const Time& time)
    : p(new SmpteTimePrivate())
{
    p->d.time = time;
    p->update();
}

SmpteTime::SmpteTime(const SmpteTime& other)
    : p(other.p)
{}

SmpteTime::~SmpteTime() {}

bool
SmpteTime::is_valid() const
{
    return p->d.hours >= 0 && p->d.hours < 24 && p->d.minutes >= 0 && p->d.minutes < 60 && p->d.seconds >= 0
           && p->d.seconds < 60 && p->d.frames >= 0 && p->d.subframes >= 0 && p->d.subframe_divisor > 0;
}

quint32
SmpteTime::counter() const
{
    return p->d.counter;
}

qint16
SmpteTime::hours() const
{
    return p->d.hours;
}

qint16
SmpteTime::minutes() const
{
    return p->d.minutes;
}

qint16
SmpteTime::seconds() const
{
    return p->d.seconds;
}

qint16
SmpteTime::frames() const
{
    return p->d.frames;
}

qint16
SmpteTime::subframes() const
{
    return p->d.subframes;
}

qint16
SmpteTime::subframe_divisor() const
{
    return p->d.subframe_divisor;
}

qint64
SmpteTime::frame() const
{
    return p->frame();
}

Time
SmpteTime::time() const
{
    return p->d.time;
}

bool
SmpteTime::negatives() const
{
    return p->d.negatives;
}

void
SmpteTime::set_time(const Time& time)
{
    p.detach();
    p->d.time = time;
    p->update();
}

void
SmpteTime::set_negatives(bool negatives)
{
    if (p->d.negatives != negatives) {
        p.detach();
        p->d.negatives = negatives;
        p->update();
    }
}

void
SmpteTime::set_fullhours(bool fullhours)
{
    if (p->d.fullhours != fullhours) {
        p.detach();
        p->d.fullhours = fullhours;
        p->update();
    }
}

QString
SmpteTime::to_string() const
{
    QString text;
    if (p->d.time.fps().drop_frame()) {
        text = "%1:%2:%3.%4";  // use . for drop frames
    }
    else {
        text = "%1:%2:%3:%4";
    }
    return QString(text)
        .arg(p->d.hours, 2, 10, QChar('0'))
        .arg(p->d.minutes, 2, 10, QChar('0'))
        .arg(p->d.seconds, 2, 10, QChar('0'))
        .arg(p->d.frames, 2, 10, QChar('0'));
}

void
SmpteTime::reset()
{
    p.detach();
    p->d.time.reset();
}

SmpteTime&
SmpteTime::operator=(const SmpteTime& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
SmpteTime::operator==(const SmpteTime& other) const
{
    return p->d.counter == other.p->d.counter && p->d.hours == other.p->d.hours && p->d.minutes == other.p->d.minutes
           && p->d.seconds == other.p->d.seconds && p->d.frames == other.p->d.frames
           && p->d.subframes == other.p->d.subframes && p->d.subframe_divisor == other.p->d.subframe_divisor;
}

bool
SmpteTime::operator!=(const SmpteTime& other) const
{
    return !(*this == other);
}

bool
SmpteTime::operator<(const SmpteTime& other) const
{
    return this->frame() < other.frame();
}

bool
SmpteTime::operator>(const SmpteTime& other) const
{
    return this->frame() > other.frame();
}

bool
SmpteTime::operator<=(const SmpteTime& other) const
{
    return this->frame() <= other.frame();
}

bool
SmpteTime::operator>=(const SmpteTime& other) const
{
    return this->frame() >= other.frame();
}

SmpteTime
SmpteTime::operator+(const SmpteTime& other) const
{
    Q_ASSERT("fps must match" && p->d.time.fps() == other.time().fps());
    qint64 frames = this->time().frames() + other.time().frames();
    Time time = Time(frames, p->d.time.fps());
    return SmpteTime(time);
}

SmpteTime
SmpteTime::operator-(const SmpteTime& other) const
{
    Q_ASSERT("fps must match" && p->d.time.fps() == other.time().fps());
    qint64 frames = this->time().frames() - other.time().frames();
    Time time = Time(frames, p->d.time.fps());
    return SmpteTime(time);
}

qint64
SmpteTime::convert(quint64 frame, const Fps& from, const Fps& to)
{
    if (from != to) {
        if (from.drop_frame() && !to.drop_frame()) {
            frame = SmpteTime::convert(frame, from, true);
        }
        if (from.framequanta() != to.framequanta()) {
            Fps from_fq = from;
            Fps to_fq = to;
            if (from.drop_frame()) {
                from_fq = Fps(from.framequanta(), 1);
            }
            if (to.drop_frame()) {
                to_fq = Fps(to.framequanta(), 1);
            }
            frame = Fps::convert(frame, from_fq, to_fq);
        }
        if (!from.drop_frame() && to.drop_frame()) {
            frame = SmpteTime::convert(frame, to, false);
        }
    }
    return frame;
}

qint64
SmpteTime::convert(quint64 frame, const Fps& fps, bool reverse)
{
    qint64 frametime = frame;
    if (!reverse) {
        if (fps.drop_frame()) {
            qint16 framequanta = fps.framequanta();
            qint64 framemin = framequanta * 60;
            qint64 frame10min = framemin * 10 - 9 * 2;
            qint64 num10s = frame / frame10min;
            qint64 adjust = -num10s * (9 * 2);
            qint64 left = frame % frame10min;
            if (left > 1) {
                qint64 num1s = left / framemin;
                if (num1s > 0) {
                    adjust -= (num1s - 1) * 2;
                    left %= framemin;

                    if (left > 1) {
                        left = qBound(qint64(0), left - 2, framemin);
                    }
                    else {
                        left = qBound(qint64(0), left - (left + 1), framemin);
                    }
                }
            }
            frametime += adjust;
        }
    }
    else {
        if (fps.drop_frame()) {
            qint64 framemin = fps.framequanta() * 60;
            qint64 frame10min = framemin * 10 - 9 * 2;
            qint64 frame10minblocks = frametime / frame10min;
            qint64 remaining = frametime % frame10min;
            qint64 adjust = frame10minblocks * (9 * 2);
            if (remaining >= framemin) {
                adjust += (remaining / framemin) * 2;
            }
            frametime = qBound(qint64(0), frametime + adjust, std::numeric_limits<qint64>::max());
        }
    }
    return frametime;
}
}  // namespace av
