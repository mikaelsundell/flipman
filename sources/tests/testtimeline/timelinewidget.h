// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include <flipmansdk/av/timeline.h>

#include <QScopedPointer>
#include <QWidget>

namespace flipman {
class TimeLineWidgetPrivate;
class TimeLineWidget : public QWidget {
    Q_OBJECT
public:
    TimeLineWidget(QWidget* parent = nullptr);
    virtual ~TimeLineWidget();

    void setTimeLine(sdk::av::Timeline* timeLine);
    void setZoom(double zoom);

Q_SIGNALS:
    void timeChanged(const sdk::av::Time& time);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    QScopedPointer<TimeLineWidgetPrivate> p;
};
}  // namespace flipman
