// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/widgets/timeedit.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPointer>

namespace flipman::sdk::widgets {
class TimeEditPrivate : public QObject {
public:
    TimeEditPrivate();
    ~TimeEditPrivate();
    void init();
    bool eventFilter(QObject* obj, QEvent* event);

    struct Data {
        av::Time time;
        TimeEdit::TimeCode timeCode = TimeEdit::TimeCode::Time;
        bool focused = false;
    };
    Data d;
    QPointer<TimeEdit> widget;
};

TimeEditPrivate::TimeEditPrivate() {}

TimeEditPrivate::~TimeEditPrivate() {}

void
TimeEditPrivate::init()
{
    widget->installEventFilter(this);
}

bool
TimeEditPrivate::eventFilter(QObject* object, QEvent* event)
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

TimeEdit::TimeEdit(QWidget* parent)
    : QWidget(parent)
    , p(new TimeEditPrivate())
{
    p->widget = this;
    p->init();
}

TimeEdit::~TimeEdit() {}

av::Time
TimeEdit::time() const
{
    return p->d.time;
}

TimeEdit::TimeCode
TimeEdit::timeCode() const
{
    return p->d.timeCode;
}

void
TimeEdit::setTime(const av::Time& time)
{
    if (p->d.time != time) {
        p->d.time = time;
        update();
    }
}

void
TimeEdit::setTimeCode(TimeEdit::TimeCode timeCode)
{
    if (p->d.timeCode != timeCode) {
        p->d.timeCode = timeCode;
        update();
    }
}

void
TimeEdit::paintEvent(QPaintEvent* event)
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
    if (p->d.timeCode == TimeEdit::TimeCode::Frames) {
        painter.drawText(textRect, Qt::AlignCenter, QString::number(p->d.time.frames()));
    }
    else if (p->d.timeCode == TimeEdit::TimeCode::Time) {
        painter.drawText(textRect, Qt::AlignCenter, p->d.time.toString());
    }
    else if (p->d.timeCode == TimeEdit::TimeCode::SMPTE) {
        painter.drawText(textRect, Qt::AlignCenter, av::SmpteTime(p->d.time).toString());
    }
    if (p->d.focused) {
        painter.setPen(QPen(highlightColor, 4));
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}
}  // namespace flipman::sdk::widgets
