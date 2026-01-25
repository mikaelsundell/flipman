// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <av/smptetime.h>
#include <widgets/timeedit.h>

namespace widgets {
class TimeeditPrivate : public QObject {
public:
    void init();
    bool eventFilter(QObject* obj, QEvent* event);

    struct Data {
        av::Time time;
        Timeedit::Timecode timecode = Timeedit::Timecode::Time;
        bool focused = false;
    };
    Data d;
    QPointer<Timeedit> widget;
};

void
TimeeditPrivate::init()
{
    widget->installEventFilter(this);
}

bool
TimeeditPrivate::eventFilter(QObject* object, QEvent* event)
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

#include "timeedit.moc"

Timeedit::Timeedit(QWidget* parent)
    : QWidget(parent)
    , p(new TimeeditPrivate())
{
    p->widget = this;
    p->init();
}

Timeedit::~Timeedit() {}

av::Time
Timeedit::time() const
{
    return p->d.time;
}

Timeedit::Timecode
Timeedit::timecode() const
{
    return p->d.timecode;
}

void
Timeedit::set_time(const av::Time& time)
{
    if (p->d.time != time) {
        p->d.time = time;
        update();
    }
}

void
Timeedit::set_timecode(Timeedit::Timecode timecode)
{
    if (p->d.timecode != timecode) {
        p->d.timecode = timecode;
        update();
    }
}

void
Timeedit::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    QPalette palette = this->palette();
    QColor backgroundColor = palette.color(QPalette::Window);
    QColor textColor = palette.color(QPalette::WindowText);
    QColor highlightColor = palette.color(QPalette::Highlight);  // System highlight color

    painter.fillRect(rect(), backgroundColor);

    QFont font = this->font();
    painter.setFont(font);
    painter.setPen(textColor);

    QRect textRect = rect().adjusted(5, 5, -5, -5);

    if (p->d.timecode == Timeedit::Timecode::Frames) {
        painter.drawText(textRect, Qt::AlignCenter, QString::number(p->d.time.frames()));
    }
    else if (p->d.timecode == Timeedit::Timecode::Time) {
        painter.drawText(textRect, Qt::AlignCenter, p->d.time.to_string());
    }
    else if (p->d.timecode == Timeedit::Timecode::SMPTE) {
        painter.drawText(textRect, Qt::AlignCenter, av::SmpteTime(p->d.time).to_string());
    }
    if (p->d.focused) {
        painter.setPen(QPen(highlightColor, 4));
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}
}  // namespace widgets
