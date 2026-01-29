// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/av/fps.h>

#include <QVector>

namespace flipman::sdk::av {
class FpsPrivate : public QSharedData {
public:
    struct Data {
        qint32 numerator = 0;
        qint32 denominator = 0;
        bool dropFrame = false;
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
    p->d.dropFrame = drop_frame;
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
Fps::dropFrame() const
{
    return p->d.dropFrame;
}

qint16
Fps::frameQuanta() const
{
    return static_cast<qint64>(std::round(real()));
}

qint32
Fps::frameScale() const
{
    return frameQuanta() * 1000;
}

qreal
Fps::real() const
{
    Q_ASSERT("fps is not valid" && isValid());

    return static_cast<qreal>(numerator()) / denominator();
}

qreal
Fps::seconds() const
{
    return 1.0 / real();
}

qreal
Fps::toFps(qint64 frame, const Fps& other) const
{
    return static_cast<qint64>(std::round(frame * (real() / other.real())));
}

QString
Fps::toString() const
{
    return QString("%1").arg(seconds());
}

void
Fps::reset()
{
    p.reset(new FpsPrivate());
}

bool
Fps::isValid() const
{
    return p->d.denominator > 0;
}

void
Fps::setNumerator(qint32 numerator)
{
    if (p->d.numerator != numerator) {
        p.detach();
        p->d.numerator = numerator;
    }
}

void
Fps::setDenominator(qint32 denominator)
{
    if (p->d.denominator != denominator) {
        if (denominator > 0) {
            p.detach();
            p->d.denominator = denominator;
        }
    }
}

void
Fps::setDropFrame(bool dropFrame)
{
    if (p->d.dropFrame != dropFrame) {
        p->d.dropFrame = dropFrame;
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
           && p->d.dropFrame == other.p->d.dropFrame;
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
    const QVector<Fps> standards = { fps23_976(), fps24(), fps25(), fps29_97(), fps30(),
                                     fps47_952(), fps48(), fps50(), fps59_94(), fps60() };
    for (const Fps& standard : standards) {
        if (qAbs(standard.real() - fps) < epsilon) {
            return standard;
        }
    }
    return Fps(static_cast<qint32>(fps * 1000), 1000);
}

Fps
Fps::fps23_976()
{
    return Fps(24000, 1001, true);
}

Fps
Fps::fps24()
{
    return Fps(24, 1);
}

Fps
Fps::fps25()
{
    return Fps(25, 1);
}

Fps
Fps::fps29_97()
{
    return Fps(30000, 1001, true);
}

Fps
Fps::fps30()
{
    return Fps(30, 1);
}

Fps
Fps::fps47_952()
{
    return Fps(48000, 1001, true);
}

Fps
Fps::fps48()
{
    return Fps(48, 1);
}

Fps
Fps::fps50()
{
    return Fps(50, 1);
}

Fps
Fps::fps59_94()
{
    return Fps(60000, 1001, true);
}

Fps
Fps::fps60()
{
    return Fps(60, 1);
}

qint64
Fps::convert(quint64 value, const Fps& from, const Fps& to)
{
    return qRound(static_cast<qreal>(value) * (to / from));
}
}  // namespace flipman::sdk::av
