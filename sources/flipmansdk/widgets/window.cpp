// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/widgets/window.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>
#include <QEvent>
#include <QPointer>
#include <QTimer>

namespace flipman::sdk::widgets {
class WindowPrivate : public QObject {
public:
    WindowPrivate();
    ~WindowPrivate();
    void init();
    bool eventFilter(QObject* obj, QEvent* event);
    struct Data {
        QPointer<Window> window;
    };
    Data d;

};

WindowPrivate::WindowPrivate() {}

WindowPrivate::~WindowPrivate() {}

void
WindowPrivate::init()
{
    d.window->installEventFilter(this);
}

bool
WindowPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (object == d.window && event->type() == QEvent::Show) {
        d.window->removeEventFilter(this);
        QTimer::singleShot(0, d.window, []() {
            sdk::core::style()->update();
        });
    }
    return QObject::eventFilter(object, event);
}

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}

}  // namespace flipman::sdk::widgets
