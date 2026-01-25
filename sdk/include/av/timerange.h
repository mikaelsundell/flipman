// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/time.h>
#include <core/object.h>

#include <QExplicitlySharedDataPointer>

namespace av {
class TimeRangePrivate;
class TimeRange : public core::Object {
public:
    TimeRange();
    TimeRange(Time start, Time duration);
    TimeRange(const TimeRange& other);
    virtual ~TimeRange();
    bool is_valid() const override;
    Time start() const;
    Time duration() const;
    Time end() const;
    Time bound(const Time& time);
    Time bound(const Time& time, bool loop = false);
    bool intersects(const TimeRange& other) const;
    QString to_string() const;
    void reset() override;

    void set_start(Time start);
    void set_duration(Time duration);

    TimeRange& operator=(const TimeRange& other);
    bool operator==(const TimeRange& other) const;
    bool operator!=(const TimeRange& other) const;

    static TimeRange convert(const TimeRange& timerange, const Fps& to);
    static TimeRange convert(const TimeRange& timerange, qint32 timescale = 24000);

private:
    QExplicitlySharedDataPointer<TimeRangePrivate> p;
};
}  // namespace av
