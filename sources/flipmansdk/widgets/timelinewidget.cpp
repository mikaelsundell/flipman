// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <av/smptetime.h>
#include <widgets/timelinewidget.h>

namespace flipman::sdk::widgets {
class TimelineWidgetPrivate {
public:
    void init();
    int mapToX(qint64 ticks) const;
    qint64 mapToTicks(int x) const;
    int mapToWidth(qint64 ticks) const;
    QSize mapToSize(const QFont& font, const QString& text) const;
    int mapToText(qint64 start, qint64 duration, int x, int width) const;
    QString labelTick(qint64 ticks) const;
    QSize labelSize(const QFont& font, qint64 ticks) const;
    qint64 ticks(qreal value) const;
    qint64 steps(qint64 value);
    qint64 substeps(qint64 steps) const;
    qint64 substeps(qint64 value, qint64 max, qint64 steps, int width) const;
    qint64 labelsteps(qint64 steps) const;
    void paintTick(QPainter& p, int x, int y, int height, qreal width = 1, QBrush brush = Qt::white);
    void paintText(QPainter& p, int x, int y, qint64 value, qint64 start, qint64 duration, bool bold = false,
                   QBrush brush = Qt::white);
    void paintTimeline(QPainter& p);
    QPixmap paint();
    struct Time {
        qreal top;
        qreal hours;
        qreal minutes;
        qreal seconds;
    };
    Time t;
    struct Data {
        av::Time time;
        av::TimeRange range;
        TimelineWidget::TimeCode timeCode = TimelineWidget::TimeCode::Time;
        qint64 lastTick = -1;
        qreal marginTick = 0.5;
        int marginRange = 10;
        int disttick = 5;
        bool tracking = false;
        bool pressed = false;
        int radius = 2;
    };
    Data d;
    QPointer<TimelineWidget> widget;
};

void
TimelineWidgetPrivate::init()
{
    widget->setAttribute(Qt::WA_TranslucentBackground);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

int
TimelineWidgetPrivate::mapToX(qint64 ticks) const
{
    qreal ratio = static_cast<qreal>(ticks) / d.range.duration().ticks();
    return qRound(d.marginRange + ratio * (widget->width() - 2 * d.marginRange));
}

qint64
TimelineWidgetPrivate::mapToTicks(int x) const
{
    x = qBound(d.marginRange, x, widget->width() - d.marginRange);
    qreal ratio = static_cast<qreal>(x - d.marginRange) / (widget->width() - 2 * d.marginRange);
    return static_cast<qint64>(ratio * (d.range.duration().ticks() - 1));
}

int
TimelineWidgetPrivate::mapToWidth(qint64 ticks) const
{
    return mapToX(ticks) - mapToX(0);
}

QSize
TimelineWidgetPrivate::mapToSize(const QFont& font, const QString& text) const
{
    QFontMetrics metrics = QFontMetrics(font);
    return QSize(metrics.horizontalAdvance(text), metrics.height());
}

int
TimelineWidgetPrivate::mapToText(qint64 start, qint64 duration, int x, int width) const
{
    int min = mapToX(start);
    int max = mapToX(duration) - width;
    return (x - width / 2);  // todo: needed? qBound(min, x - width / 2, max);
}

QString
TimelineWidgetPrivate::labelTick(qint64 value) const
{
    if (d.timeCode == TimelineWidget::TimeCode::Frames) {
        return QString::number(d.time.frame(value));
    }
    else if (d.timeCode == TimelineWidget::TimeCode::Time) {
        return d.range.duration().toString(value);
    }
    else if (d.timeCode == TimelineWidget::TimeCode::SMPTE) {
        return av::SmpteTime(av::Time(d.range.duration(), value)).toString();
    }
}

QSize
TimelineWidgetPrivate::labelSize(const QFont& font, qint64 value) const
{
    if (d.timeCode == TimelineWidget::TimeCode::Frames) {
        return mapToSize(font, QString::number(d.range.duration().frames()));
    }
    else if (d.timeCode == TimelineWidget::TimeCode::Time) {
        return mapToSize(font, d.range.duration().toString(value));
    }
    else if (d.timeCode == TimelineWidget::TimeCode::SMPTE) {
        return mapToSize(font, av::SmpteTime(av::Time(d.range.duration(), value)).toString());
    }
}

qint64
TimelineWidgetPrivate::ticks(qreal value) const
{
    if (d.timeCode == TimelineWidget::TimeCode::Frames) {
        return d.range.duration().ticks(static_cast<qint64>(value));
    }
    else if (d.timeCode == TimelineWidget::TimeCode::Time || d.timeCode == TimelineWidget::TimeCode::SMPTE) {
        return d.range.duration().timeScale() * value;
    }
}

qint64
TimelineWidgetPrivate::steps(qint64 value)
{
    return pow(10, qint64(log10(value)));
}

qint64
TimelineWidgetPrivate::substeps(qint64 value, qint64 top, qint64 steps, int limit) const
{
    qint64 max = top * ticks(1);
    for (qint64 tick = steps; tick < max; tick += steps) {
        qint64 substeps = (tick / ticks(1));
        if (top % substeps == 0) {
            if (mapToWidth(tick) * d.marginTick > limit) {
                return tick;
            }
        }
    }
    return steps;
}

void
TimelineWidgetPrivate::paintTick(QPainter& p, int x, int y, int height, qreal width, QBrush brush)
{
    p.save();
    p.setPen(QPen(brush, width));
    p.drawLine(x, y - height, x, y + height);
    p.restore();
}

void
TimelineWidgetPrivate::paintText(QPainter& p, int x, int y, qint64 value, qint64 start, qint64 duration, bool bold,
                                 QBrush brush)
{
    p.save();
    p.setPen(QPen(brush, 1));
    if (bold) {
        QFont font = p.font();
        font.setBold(true);
        font.setPointSize(8);
        p.setFont(font);
    }
    else {
        QFont font = p.font();
        font.setPointSize(7);
        p.setFont(font);
    }
    QString text = labelTick(value);
    QSize size = mapToSize(p.font(), text);
    p.drawText(mapToText(start, duration, x, size.width()), y + 4 + size.height(), text);
    p.restore();
}

void
TimelineWidgetPrivate::paintTimeline(QPainter& p)
{
    p.save();
    QFont font("Courier New", 8);  // fixed font size
    p.setFont(font);
    p.setPen(QPen(Qt::white, 1));
    int y = widget->height() / 2;

    qint64 start = d.range.start().ticks();
    qint64 duration = d.range.duration().ticks();
    int limit = labelSize(font, duration).width();

    qint64 hoursteps = ticks(t.hours);
    if (hoursteps) {
        qint64 hoursubsteps = 0;
        if (!(mapToWidth(hoursteps) * d.marginTick > limit)) {
            hoursubsteps = substeps(t.hours, t.top, hoursteps, limit);
        }
        for (qint64 hour = start; hour < duration; hour += hoursteps) {
            int x = mapToX(hour);
            if (hoursubsteps > 0) {
                if (hour % hoursubsteps == 0) {
                    if (mapToWidth(hoursubsteps) > d.disttick) {
                        paintTick(p, x, y, 2, 2);
                    }
                    if (mapToWidth(hoursubsteps) * d.marginTick > limit && hour > start) {
                        paintText(p, x, y, hour, start, duration, true);
                    }
                }
                if (mapToWidth(hoursubsteps) > d.disttick) {
                    paintTick(p, x, y, 2, 2);
                }
            }
            else {
                paintTick(p, x, y, 2, 2);
                if (hour > start) {
                    paintText(p, x, y, hour, start, duration, true);
                }
            }
            qint64 minutessteps = ticks(t.minutes);
            if (minutessteps) {
                qint64 minutesubsteps = 0;
                if (!(mapToWidth(minutessteps) * d.marginTick > limit)) {
                    minutesubsteps = substeps(t.minutes, t.hours, minutessteps, limit);
                }
                for (qint64 minute = hour; minute < (hour + hoursteps) && minute < duration; minute += minutessteps) {
                    int x = mapToX(minute);
                    if (minutesubsteps > 0) {
                        if (minute % minutesubsteps == 0) {
                            if (mapToWidth(minutesubsteps) > d.disttick) {
                                paintTick(p, x, y, 2, 1);
                            }
                            if (mapToWidth(minutesubsteps) * d.marginTick > limit && minute > hour) {
                                paintText(p, x, y, minute, start, duration);
                            }
                        }
                        if (mapToWidth(minutessteps) > d.disttick) {
                            paintTick(p, x, y, 2, 1);
                        }
                    }
                    else {
                        paintTick(p, x, y, 2, 1);
                        if (minute > hour) {
                            paintText(p, x, y, minute, start, duration);
                        }
                    }
                    qint64 secondssteps = ticks(t.seconds);
                    if (secondssteps) {
                        qint64 secondssubsteps = 0;
                        if (!(mapToWidth(secondssteps) * d.marginTick > limit)) {
                            secondssubsteps = substeps(t.seconds, t.minutes, secondssteps, limit);
                        }
                        for (qint64 second = minute; second < (minute + minutessteps) && second < duration;
                             second += secondssteps) {
                            int x = mapToX(second);
                            if (secondssubsteps > 0) {
                                if (second % secondssubsteps == 0) {
                                    if (mapToWidth(secondssubsteps) > d.disttick) {
                                        paintTick(p, x, y, 2, 1);
                                    }
                                    if (mapToWidth(secondssubsteps) * d.marginTick > limit && second > minute) {
                                        paintText(p, x, y, second, start, duration);
                                    }
                                }
                                if (mapToWidth(secondssteps) > d.disttick) {
                                    paintTick(p, x, y, 2, 1);
                                }
                            }
                            else {
                                paintTick(p, x, y, 2, 1);
                                if (second > minute) {
                                    paintText(p, x, y, second, start, duration);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    p.restore();
}

QPixmap
TimelineWidgetPrivate::paint()
{
    qreal dpr = widget->devicePixelRatio();
    QPixmap pixmap(widget->width() * dpr, widget->height() * dpr);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(dpr);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QFont font("Courier New", 11);  // fixed font size
    p.setFont(font);

    int y = widget->height() / 2;
    p.setPen(QPen(Qt::lightGray, 1));
    p.drawLine(0, y, widget->width(), y);
    p.drawLine(d.marginRange, y, widget->width() - d.marginRange, y);
    font.setPointSizeF(font.pointSizeF() * 0.8f);
    p.setFont(font);

    if (d.range.isValid()) {
        p.setPen(QPen(Qt::white, 1));
        // timeline
        {
            qint64 ticks = d.time.ticks();
            qint64 start = d.range.start().ticks();
            qint64 duration = d.range.duration().ticks();
            if (d.timeCode == TimelineWidget::TimeCode::Frames) {
                qint64 frames = d.range.duration().frames();
                t = Time {
                    // redistribute to frames
                    static_cast<qreal>(steps(frames) * 10),
                    static_cast<qreal>(steps(frames)),
                    static_cast<qreal>(steps(frames) / 10),
                    static_cast<qreal>(steps(frames) / 10 / 10),
                };
            }
            else if (d.timeCode == TimelineWidget::TimeCode::Time || d.timeCode == TimelineWidget::TimeCode::SMPTE) {
                av::Time duration = d.range.duration();
                qint64 seconds = duration.seconds();
                qint64 minutes = seconds / 60;
                qint64 hours = minutes / 60;
                if (hours > 0) {
                    t = Time {
                        60 * 60 * 24,
                        60 * 60,
                        60,
                        1,
                    };
                }
                else if (minutes > 0) {
                    t = Time { // redistribute to mins
                               60 * 60, 60, 1, 1.0 / duration.fps().frameQuanta()
                    };
                }
                else {
                    t = Time { // redistribute to secs
                               60, 1, 1.0 / duration.fps().frameQuanta(), 0
                    };
                }
            }
            paintTimeline(p);
            {
                int pos = mapToX(start);
                QPen thickPen(Qt::white, 4);
                p.setPen(thickPen);
                p.drawLine(pos, y - 4, pos, y + 4);
            }
            {
                int pos = mapToX(duration);
                QPen thickPen(Qt::white, 4);
                p.setPen(thickPen);
                p.drawLine(pos, y - 4, pos, y + 4);
            }
            if (d.pressed) {
                QString text = labelTick(ticks);
                int pos = mapToX(ticks);
                QFontMetrics metrics(font);
                int pad = 4;
                int textw = metrics.horizontalAdvance(QString().fill('0', text.size())) + 2 * pad;
                int texth = metrics.height() + 2 * pad;
                int min = mapToX(start) + textw / 2 - d.marginRange / 2;
                int max = mapToX(duration) - textw / 2 + d.marginRange / 2;
                int bound = qBound(min, pos, max);
                QRect rect(bound - textw / 2, y - texth / 2, textw, texth);
                p.setPen(Qt::black);
                p.setBrush(Qt::gray);
                p.drawRect(rect);
                p.drawText(rect, Qt::AlignCenter, text);
            }
            else {
                int pos = 0;
                av::Time next = av::Time(d.time, ticks + d.time.tpf());
                if (next.ticks() < duration) {
                    pos = mapToX(next.ticks());
                    p.setPen(QPen(Qt::darkRed, 2));
                    p.setBrush(Qt::darkRed);
                    p.drawLine(pos, y - 2, pos, y + 2);
                    p.drawLine(pos, y, mapToX(ticks), y);
                }
                pos = mapToX(ticks);
                p.setPen(QPen(Qt::white, 4));
                p.setBrush(Qt::white);
                p.drawLine(pos, y - 4, pos, y + 4);

                p.setPen(Qt::NoPen);
                p.setBrush(Qt::red);
                p.drawEllipse(QPoint(pos, y), d.radius, d.radius);
            }
        }
    }
    return pixmap;
}

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent)
    , p(new TimelineWidgetPrivate())
{
    p->widget = this;
    p->init();
}

TimelineWidget::~TimelineWidget() {}

QSize
TimelineWidget::sizeHint() const
{
    return QSize(200, 35);
}

av::TimeRange
TimelineWidget::range() const
{
    return p->d.range;
}

av::Time
TimelineWidget::time() const
{
    return p->d.time;
}

bool
TimelineWidget::tracking() const
{
    return p->d.tracking;
}

TimelineWidget::TimeCode
TimelineWidget::timeCode() const
{
    return p->d.timeCode;
}

void
TimelineWidget::setRange(const av::TimeRange& range)
{
    if (p->d.range != range) {
        p->d.range = range;
        update();
    }
}

void
TimelineWidget::setTime(const av::Time& time)
{
    if (p->d.time != time && !p->d.pressed) {
        p->d.time = time;
        timeChanged(time);
        update();
    }
}

void
TimelineWidget::setTracking(bool tracking)
{
    if (p->d.tracking != tracking) {
        p->d.tracking = tracking;
        update();
    }
}

void
TimelineWidget::setTimeCode(TimelineWidget::TimeCode timeCode)
{
    if (p->d.timeCode != timeCode) {
        p->d.timeCode = timeCode;
        update();
    }
}

void
TimelineWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, p->paint());
    painter.end();
    QWidget::paintEvent(event);
}

void
TimelineWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::SizeHorCursor);
        qint64 pos = p->mapToTicks(event->pos().x());
        p->d.time.setTicks(pos);
        p->d.pressed = true;
        sliderPressed();
        update();
    }
}

void
TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (p->d.pressed) {
        qint64 tick = p->d.time.align(p->mapToTicks(event->pos().x()));
        if (p->d.lastTick != tick) {
            p->d.time.setTicks(tick);
            Q_EMIT sliderMoved(p->d.time);
            if (p->d.tracking) {
                Q_EMIT timeChanged(p->d.time);
            }
            p->d.lastTick = tick;
            update();
        }
    }
}

void
TimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (p->d.pressed && event->button() == Qt::LeftButton) {
        setCursor(Qt::ArrowCursor);
        p->d.pressed = false;
        sliderReleased();
        update();
    }
}
}  // namespace flipman::sdk::widgets
