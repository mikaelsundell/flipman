// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/fps.h>
#include <core/object.h>

#include <QExplicitlySharedDataPointer>

namespace av {
class TimePrivate;
class Time : public core::Object {
public:
    Time();
    Time(qint64 ticks, qint32 timescale, const Fps& fps);
    Time(qint64 frame, const Fps& fps);
    Time(qreal seconds, const Fps& fps);
    Time(const Time& other, qint64 ticks);
    Time(const Time& other, const Fps& fps);
    Time(const Time& other);
    virtual ~Time();
    bool is_valid() const override;
    Fps fps() const;
    qint64 ticks() const;
    qint64 ticks(qint64 frame) const;
    qint32 timescale() const;
    qint64 tpf() const;
    qint64 frame(qint64 ticks) const;
    qint64 lastframe() const;
    qint64 frames() const;
    qint64 align(qint64 ticks) const;
    qreal seconds() const;
    QString to_string(qint64 ticks) const;
    QString to_string() const;
    void reset() override;

    void set_ticks(qint64 ticks);
    void set_timescale(qint32 timescale);
    void set_fps(const Fps& fps);

    Time& operator=(const Time& other);
    bool operator==(const Time& other) const;
    bool operator!=(const Time& other) const;
    bool operator<(const Time& other) const;
    bool operator>(const Time& other) const;
    bool operator<=(const Time& other) const;
    bool operator>=(const Time& other) const;
    Time operator+(const Time& other) const;
    Time operator-(const Time& other) const;
    operator double() const;

    static Time convert(const Time& time, const Fps& to);
    static Time convert(const Time& time, qint32 timescale = 24000);

private:
    QExplicitlySharedDataPointer<TimePrivate> p;
};
}  // namespace av
