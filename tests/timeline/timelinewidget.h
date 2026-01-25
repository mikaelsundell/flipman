// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include <QScopedPointer>
#include <QWidget>
#include <av/timeline.h>

class TimelineWidgetPrivate;
class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    TimelineWidget(QWidget* parent = nullptr);
    virtual ~TimelineWidget();

    void set_timeline(av::Timeline* timeline);
    void set_zoom(double zoom);

Q_SIGNALS:
    void time_changed(const av::Time& time);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    QScopedPointer<TimelineWidgetPrivate> p;
};
