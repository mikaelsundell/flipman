// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QExplicitlySharedDataPointer>
#include <av/fps.h>
#include <av/time.h>

namespace av {
class SmpteTimePrivate;
class SmpteTime : public core::Object {
public:
    SmpteTime();
    SmpteTime(const Time& time);
    SmpteTime(const SmpteTime& other);
    virtual ~SmpteTime();
    bool is_valid() const override;
    quint32 counter() const;
    qint16 hours() const;
    qint16 minutes() const;
    qint16 seconds() const;
    qint16 frames() const;
    qint16 subframes() const;
    qint16 subframe_divisor() const;
    qint64 frame() const;
    Time time() const;
    bool negatives() const;
    bool fullhours() const;
    QString to_string() const;
    void reset() override;

    void set_time(const Time& time);
    void set_negatives(bool negatives);
    void set_fullhours(bool fullhours);

    SmpteTime& operator=(const SmpteTime& other);
    bool operator==(const SmpteTime& other) const;
    bool operator!=(const SmpteTime& other) const;
    bool operator<(const SmpteTime& other) const;
    bool operator>(const SmpteTime& other) const;
    bool operator<=(const SmpteTime& other) const;
    bool operator>=(const SmpteTime& other) const;
    SmpteTime operator+(const SmpteTime& other) const;
    SmpteTime operator-(const SmpteTime& other) const;

    static qint64 convert(quint64 frame, const Fps& from, const Fps& to);
    static qint64 convert(quint64 frame, const Fps& fps, bool reverse = false);

private:
    QExplicitlySharedDataPointer<SmpteTimePrivate> p;
};
}  // namespace av
