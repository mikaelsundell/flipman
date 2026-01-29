// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

namespace flipman {
class WindowPrivate;
class Window : public QMainWindow {
public:
    Window(QWidget* parent = nullptr);
    virtual ~Window();
    
private:
    QScopedPointer<WindowPrivate> p;
};
}
