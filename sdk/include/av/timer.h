// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QScopedPointer>
#include <av/fps.h>
#include <core/object.h>

namespace av {
class TimerPrivate;
class Timer : public core::Object {
public:
    enum Unit { Nanos, Seconds, Minutes, Hours };
    Timer();
    virtual ~Timer();
    bool is_valid() const override;
    void start();
    void start(const Fps& fps);
    void stop();
    void restart();
    void lap();
    bool next(const Fps& fps);
    void wait();
    void sleep(quint64 msecs);
    quint64 elapsed() const;
    QList<quint64> laps() const;
    void reset() override;

    static qreal convert(quint64 nano, Unit unit = Unit::Nanos);

private:
    QScopedPointer<TimerPrivate> p;
};
}  // namespace av
