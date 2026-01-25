// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

class WindowPrivate;
class Window : public QMainWindow {
    Q_OBJECT
public:
    Window(QWidget* parent = nullptr);
    virtual ~Window();

private:
    QScopedPointer<WindowPrivate> p;
};
