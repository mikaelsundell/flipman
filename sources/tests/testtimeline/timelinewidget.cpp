// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "timelinewidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>

class TimelineWidgetPrivate : public QObject {
    Q_OBJECT
public:
    void init();
    struct Data {
        double zoom = 1.0;
        double currentTime = 0.0;
        bool draggingTimeIndicator = false;
        QPointer<av::Timeline> timeline;
        QPointer<TimelineWidget> widget;
    };
    Data d;
};

void
TimelineWidgetPrivate::init()
{}

#include "timelinewidget.moc"

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent)
    , p(new TimelineWidgetPrivate())
{
    p->d.widget = this;
    p->init();
}

TimelineWidget::~TimelineWidget() {}

void
TimelineWidget::set_timeline(av::Timeline* timeline)
{
    p->d.timeline = timeline;
    update();
}

void
TimelineWidget::set_zoom(double zoom)
{
    p->d.zoom = zoom;
    update();
}

void
TimelineWidget::paintEvent(QPaintEvent* event)
{
    if (!p->d.timeline) {
        return QWidget::paintEvent(event);
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int headerHeight = 30;
    int tickHeight = 10;
    int labelOffset = 5;
    int trackHeight = 50;
    int gridLineThickness = 1;
    int boxPadding = 4;

    float zoom = p->d.zoom > 0 ? p->d.zoom : 1.0f;
    int pixelsPerSecond = static_cast<int>(100 * zoom);

    av::TimeRange range = p->d.timeline->timerange();
    int startTime = range.start().seconds();
    int endTime = range.end().seconds();

    painter.fillRect(0, 0, width(), headerHeight, Qt::lightGray);

    for (int sec = startTime; sec <= endTime; ++sec) {
        int x = (sec - startTime) * pixelsPerSecond;
        painter.setPen(Qt::black);
        painter.drawLine(x, headerHeight - tickHeight, x, headerHeight);

        QString label = QString::number(sec);
        QRect textRect(x + labelOffset, 0, pixelsPerSecond, headerHeight);
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);
    }
    qsizetype numTracks = p->d.timeline->tracks().size();
    const QList<av::Track*> tracks = p->d.timeline->tracks();

    for (int i = 0; i < numTracks; ++i) {
        int y = headerHeight + i * trackHeight;
        painter.setPen(QPen(Qt::gray, gridLineThickness, Qt::SolidLine));
        painter.drawLine(0, y, width(), y);

        av::Track* track = tracks[i];
        for (av::Clip* clip : track->clips()) {
            av::TimeRange timerange = track->cliprange(clip);

            int startX = (timerange.start().seconds() - startTime) * pixelsPerSecond;
            int endX = (timerange.end().seconds() - startTime) * pixelsPerSecond;
            int boxWidth = endX - startX;

            startX += boxPadding;
            boxWidth -= 2 * boxPadding;
            int boxY = y + boxPadding;
            int boxHeight = trackHeight - 2 * boxPadding;

            painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
            painter.setBrush(track->color());
            painter.drawRoundedRect(startX, boxY, boxWidth, boxHeight, 4, 4);

            painter.setPen(Qt::black);
            QString label = clip->name();
            painter.drawText(startX + 5, boxY + boxHeight / 2 + 5, label);
        }
    }
    painter.setPen(QPen(Qt::gray, gridLineThickness, Qt::SolidLine));
    painter.drawLine(0, headerHeight + numTracks * trackHeight, width(), headerHeight + numTracks * trackHeight);

    int indicatorX = (p->d.currentTime - startTime) * pixelsPerSecond;
    painter.setPen(QPen(Qt::red, 2, Qt::SolidLine));
    painter.drawLine(indicatorX, 0, indicatorX, height());
    QWidget::paintEvent(event);
}

void
TimelineWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        float zoom = p->d.zoom > 0 ? p->d.zoom : 1.0f;
        int pixelsPerSecond = static_cast<int>(100 * zoom);

        av::TimeRange range = p->d.timeline->timerange();
        int startTime = range.start().seconds();
        double clickedTime = startTime + (event->x() / static_cast<double>(pixelsPerSecond));

        int indicatorX = (p->d.currentTime - startTime) * pixelsPerSecond;
        if (qAbs(event->x() - indicatorX) < 10) {
            p->d.draggingTimeIndicator = true;
            p->d.currentTime = clickedTime;
            emit time_changed(av::Time(p->d.currentTime, p->d.timeline->fps()));
            update();
        }
    }
    QWidget::mousePressEvent(event);
}

void
TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (p->d.draggingTimeIndicator) {
        float zoom = p->d.zoom > 0 ? p->d.zoom : 1.0f;
        int pixelsPerSecond = static_cast<int>(100 * zoom);

        av::TimeRange range = p->d.timeline->timerange();
        int startTime = range.start().seconds();
        p->d.currentTime = startTime + (event->x() / static_cast<double>(pixelsPerSecond));

        int endTime = range.end().seconds();
        p->d.currentTime = qBound(static_cast<double>(startTime), p->d.currentTime, static_cast<double>(endTime));

        emit time_changed(av::Time(p->d.currentTime, p->d.timeline->fps()));
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void
TimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && p->d.draggingTimeIndicator) {
        p->d.draggingTimeIndicator = false;
        emit time_changed(av::Time(p->d.currentTime, p->d.timeline->fps()));
    }
    QWidget::mouseReleaseEvent(event);
}

QSize
TimelineWidget::sizeHint() const
{
    if (!p->d.timeline) {
        return QWidget::sizeHint();
    }

    av::TimeRange range = p->d.timeline->timerange();
    int durationSeconds = range.duration().seconds();

    float zoom = p->d.zoom > 0 ? p->d.zoom : 1.0f;
    int pixelsPerSecond = static_cast<int>(100 * zoom);
    int w = durationSeconds * pixelsPerSecond;
    w = qMax(w, width() + 100);

    int headerHeight = 30;
    int trackHeight = 50;
    qsizetype numTracks = p->d.timeline->tracks().size();
    qsizetype height = headerHeight + numTracks * trackHeight;
    return QSize(w, height);
}
