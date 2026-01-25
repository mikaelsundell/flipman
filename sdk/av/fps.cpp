// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QVector>
#include <av/fps.h>

namespace av {
class FpsPrivate : public QSharedData {
public:
    struct Data {
        qint32 numerator = 0;
        qint32 denominator = 0;
        bool drop_frame = false;
    };
    Data d;
};

Fps::Fps()
    : p(new FpsPrivate())
{}

Fps::Fps(qint32 numerator, qint32 denominator, bool drop_frame)
    : p(new FpsPrivate())
{
    p->d.numerator = numerator;
    p->d.denominator = denominator;
    p->d.drop_frame = drop_frame;
}

Fps::Fps(const Fps& other)
    : p(other.p)
{}

Fps::~Fps() {}

qint64
Fps::numerator() const
{
    return p->d.numerator;
}

qint32
Fps::denominator() const
{
    return p->d.denominator;
}

bool
Fps::drop_frame() const
{
    return p->d.drop_frame;
}

qint16
Fps::framequanta() const
{
    return static_cast<qint64>(std::round(real()));
}

qint32
Fps::framescale() const
{
    return framequanta() * 1000;
}

qreal
Fps::real() const
{
    Q_ASSERT("fps is not valid" && is_valid());

    return static_cast<qreal>(numerator()) / denominator();
}

qreal
Fps::seconds() const
{
    return 1.0 / real();
}

qreal
Fps::to_fps(qint64 frame, const Fps& other) const
{
    return static_cast<qint64>(std::round(frame * (real() / other.real())));
}

QString
Fps::to_string() const
{
    return QString("%1").arg(seconds());
}

void
Fps::reset()
{
    p.reset(new FpsPrivate());
}

bool
Fps::is_valid() const
{
    return p->d.denominator > 0;
}

void
Fps::set_numerator(qint32 numerator)
{
    if (p->d.numerator != numerator) {
        p.detach();
        p->d.numerator = numerator;
    }
}

void
Fps::set_denominator(qint32 denominator)
{
    if (p->d.denominator != denominator) {
        if (denominator > 0) {
            p.detach();
            p->d.denominator = denominator;
        }
    }
}

void
Fps::set_dropframe(bool dropframe)
{
    if (p->d.drop_frame != dropframe) {
        p->d.drop_frame = dropframe;
    }
}

Fps&
Fps::operator=(const Fps& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
Fps::operator==(const Fps& other) const
{
    return p->d.numerator == other.p->d.numerator && p->d.denominator == other.p->d.denominator
           && p->d.drop_frame == other.p->d.drop_frame;
}

bool
Fps::operator!=(const Fps& other) const
{
    return !(*this == other);
}

bool
Fps::operator<(const Fps& other) const
{
    return real() < other.real();
}

bool
Fps::operator>(const Fps& other) const
{
    return real() > other.real();
}

bool
Fps::operator<=(const Fps& other) const
{
    return real() <= other.real();
}

bool
Fps::operator>=(const Fps& other) const
{
    return real() >= other.real();
}

Fps::operator double() const { return real(); }

Fps
Fps::guess(qreal fps)
{
    const qreal epsilon = 0.005;
    const QVector<Fps> standards = { fps_23_976(), fps_24(), fps_25(), fps_29_97(), fps_30(),
                                     fps_47_952(), fps_48(), fps_50(), fps_59_94(), fps_60() };
    for (const Fps& standard : standards) {
        if (qAbs(standard.real() - fps) < epsilon) {
            return standard;
        }
    }
    return Fps(static_cast<qint32>(fps * 1000), 1000);
}

Fps
Fps::fps_23_976()
{
    return Fps(24000, 1001, true);
}

Fps
Fps::fps_24()
{
    return Fps(24, 1);
}

Fps
Fps::fps_25()
{
    return Fps(25, 1);
}

Fps
Fps::fps_29_97()
{
    return Fps(30000, 1001, true);
}

Fps
Fps::fps_30()
{
    return Fps(30, 1);
}

Fps
Fps::fps_47_952()
{
    return Fps(48000, 1001, true);
}

Fps
Fps::fps_48()
{
    return Fps(48, 1);
}

Fps
Fps::fps_50()
{
    return Fps(50, 1);
}

Fps
Fps::fps_59_94()
{
    return Fps(60000, 1001, true);
}

Fps
Fps::fps_60()
{
    return Fps(60, 1);
}

qint64
Fps::convert(quint64 value, const Fps& from, const Fps& to)
{
    return qRound(static_cast<qreal>(value) * (to / from));
}
}  // namespace av
