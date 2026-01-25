// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include <av/time.h>

#include <QScopedPointer>
#include <QWidget>

namespace widgets {
class TimeeditPrivate;
class Timeedit : public QWidget {
    Q_OBJECT
public:
    enum Timecode { Frames, Time, SMPTE };
    Q_ENUM(Timecode)

    Timeedit(QWidget* parent = nullptr);
    virtual ~Timeedit();
    av::Time time() const;
    Timecode timecode() const;

public Q_SLOTS:
    void set_time(const av::Time& time);
    void set_timecode(Timeedit::Timecode timecode);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QScopedPointer<TimeeditPrivate> p;
};
}  // namespace widgets
