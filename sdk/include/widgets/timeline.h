// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QScopedPointer>
#include <QWidget>
#include <av/smptetime.h>
#include <av/time.h>
#include <av/timerange.h>

namespace widgets {
class TimelinePrivate;
class Timeline : public QWidget {
    Q_OBJECT
public:
    enum Timecode { Frames, Time, SMPTE };
    Q_ENUM(Timecode)

    Timeline(QWidget* parent = nullptr);
    virtual ~Timeline();
    QSize sizeHint() const override;
    av::TimeRange range() const;
    av::Time time() const;
    bool tracking() const;
    Timecode timecode() const;

public Q_SLOTS:
    void set_range(const av::TimeRange& range);
    void set_time(const av::Time& time);
    void set_tracking(bool tracking);
    void set_timecode(Timeline::Timecode timecode);

Q_SIGNALS:
    void time_changed(const av::Time& time);
    void slider_moved(const av::Time& time);  // todo: look into name here, good enough?
    void slider_pressed();
    void slider_released();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QScopedPointer<TimelinePrivate> p;
};
}  // namespace widgets
