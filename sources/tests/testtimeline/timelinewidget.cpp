// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "timelinewidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>

namespace flipman {
class TimeLineWidgetPrivate {
public:
    void init();
    struct Data {
        double zoom = 1.0;
        double currentTime = 0.0;
        bool draggingTimeIndicator = false;
        QPointer<sdk::av::Timeline> timeLine;
        QPointer<TimeLineWidget> widget;
    };
    Data d;
};

void
TimeLineWidgetPrivate::init()
{}

#include "timelinewidget.moc"

TimeLineWidget::TimeLineWidget(QWidget* parent)
: QWidget(parent)
, p(new TimeLineWidgetPrivate())
{
    p->d.widget = this;
    p->init();
}

TimeLineWidget::~TimeLineWidget() {}

void
TimeLineWidget::setTimeLine(sdk::av::Timeline* timeLine)
{
    p->d.timeLine = timeLine;
    update();
}

void
TimeLineWidget::setZoom(double zoom)
{
    p->d.zoom = zoom;
    update();
}

void
TimeLineWidget::paintEvent(QPaintEvent* event)
{
    if (!p->d.timeLine) {
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
    
    sdk::av::TimeRange range = p->d.timeLine->timeRange();
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
    qsizetype numTracks = p->d.timeLine->tracks().size();
    const QList<sdk::av::Track*> tracks = p->d.timeLine->tracks();
    
    for (int i = 0; i < numTracks; ++i) {
        int y = headerHeight + i * trackHeight;
        painter.setPen(QPen(Qt::gray, gridLineThickness, Qt::SolidLine));
        painter.drawLine(0, y, width(), y);
        
        sdk::av::Track* track = tracks[i];
        for (sdk::av::Clip* clip : track->clips()) {
            sdk::av::TimeRange timeRange = track->clipRange(clip);
            
            int startX = (timeRange.start().seconds() - startTime) * pixelsPerSecond;
            int endX = (timeRange.end().seconds() - startTime) * pixelsPerSecond;
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
TimeLineWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        float zoom = p->d.zoom > 0 ? p->d.zoom : 1.0f;
        int pixelsPerSecond = static_cast<int>(100 * zoom);
        
        sdk::av::TimeRange range = p->d.timeLine->timeRange();
        int startTime = range.start().seconds();
        double clickedTime = startTime + (event->x() / static_cast<double>(pixelsPerSecond));
        
        int indicatorX = (p->d.currentTime - startTime) * pixelsPerSecond;
        if (qAbs(event->x() - indicatorX) < 10) {
            p->d.draggingTimeIndicator = true;
            p->d.currentTime = clickedTime;
            Q_EMIT timeChanged(sdk::av::Time(p->d.currentTime, p->d.timeLine->fps()));
            update();
        }
    }
    QWidget::mousePressEvent(event);
}

void
TimeLineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (p->d.draggingTimeIndicator) {
        float zoom = p->d.zoom > 0 ? p->d.zoom : 1.0f;
        int pixelsPerSecond = static_cast<int>(100 * zoom);
        
        sdk::av::TimeRange range = p->d.timeLine->timeRange();
        int startTime = range.start().seconds();
        p->d.currentTime = startTime + (event->x() / static_cast<double>(pixelsPerSecond));
        
        int endTime = range.end().seconds();
        p->d.currentTime = qBound(static_cast<double>(startTime), p->d.currentTime, static_cast<double>(endTime));
        
        Q_EMIT timeChanged(sdk::av::Time(p->d.currentTime, p->d.timeLine->fps()));
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void
TimeLineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && p->d.draggingTimeIndicator) {
        p->d.draggingTimeIndicator = false;
        Q_EMIT timeChanged(sdk::av::Time(p->d.currentTime, p->d.timeLine->fps()));
    }
    QWidget::mouseReleaseEvent(event);
}

QSize
TimeLineWidget::sizeHint() const
{
    if (!p->d.timeLine) {
        return QWidget::sizeHint();
    }
    
    sdk::av::TimeRange range = p->d.timeLine->timeRange();
    int durationSeconds = range.duration().seconds();
    
    float zoom = p->d.zoom > 0 ? p->d.zoom : 1.0f;
    int pixelsPerSecond = static_cast<int>(100 * zoom);
    int w = durationSeconds * pixelsPerSecond;
    w = qMax(w, width() + 100);
    
    int headerHeight = 30;
    int trackHeight = 50;
    qsizetype numTracks = p->d.timeLine->tracks().size();
    qsizetype height = headerHeight + numTracks * trackHeight;
    return QSize(w, height);
}
}
