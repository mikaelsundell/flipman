// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <av/smptetime.h>
#include <widgets/timeeditwidget.h>

namespace flipman::sdk::widgets {
class TimeEditWidgetPrivate : public QObject {
public:
    TimeEditWidgetPrivate();
    ~TimeEditWidgetPrivate();
    void init();
    bool eventFilter(QObject* obj, QEvent* event);

    struct Data {
        av::Time time;
        TimeEditWidget::TimeCode timeCode = TimeEditWidget::TimeCode::Time;
        bool focused = false;
    };
    Data d;
    QPointer<TimeEditWidget> widget;
};

TimeEditWidgetPrivate::TimeEditWidgetPrivate() {}

TimeEditWidgetPrivate::~TimeEditWidgetPrivate() {}

void
TimeEditWidgetPrivate::init()
{
    widget->installEventFilter(this);
}

bool
TimeEditWidgetPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            d.focused = true;
            widget->update();
            widget->setFocus();
            return true;
        }
    }
    else if (event->type() == QEvent::FocusOut) {
        d.focused = false;
        widget->update();
        return true;
    }
    return QObject::eventFilter(object, event);
}

TimeEditWidget::TimeEditWidget(QWidget* parent)
    : QWidget(parent)
    , p(new TimeEditWidgetPrivate())
{
    p->widget = this;
    p->init();
}

TimeEditWidget::~TimeEditWidget() {}

av::Time
TimeEditWidget::time() const
{
    return p->d.time;
}

TimeEditWidget::TimeCode
TimeEditWidget::timeCode() const
{
    return p->d.timeCode;
}

void
TimeEditWidget::setTime(const av::Time& time)
{
    if (p->d.time != time) {
        p->d.time = time;
        update();
    }
}

void
TimeEditWidget::setTimeCode(TimeEditWidget::TimeCode timeCode)
{
    if (p->d.timeCode != timeCode) {
        p->d.timeCode = timeCode;
        update();
    }
}

void
TimeEditWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    QPalette palette = this->palette();
    QColor backgroundColor = palette.color(QPalette::Window);
    QColor textColor = palette.color(QPalette::WindowText);
    QColor highlightColor = palette.color(QPalette::Highlight);
    painter.fillRect(rect(), backgroundColor);
    QFont font = this->font();
    painter.setFont(font);
    painter.setPen(textColor);
    QRect textRect = rect().adjusted(5, 5, -5, -5);
    if (p->d.timeCode == TimeEditWidget::TimeCode::Frames) {
        painter.drawText(textRect, Qt::AlignCenter, QString::number(p->d.time.frames()));
    }
    else if (p->d.timeCode == TimeEditWidget::TimeCode::Time) {
        painter.drawText(textRect, Qt::AlignCenter, p->d.time.toString());
    }
    else if (p->d.timeCode == TimeEditWidget::TimeCode::SMPTE) {
        painter.drawText(textRect, Qt::AlignCenter, av::SmpteTime(p->d.time).toString());
    }
    if (p->d.focused) {
        painter.setPen(QPen(highlightColor, 4));
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}
}  // namespace flipman::sdk::widgets
