// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

namespace flipman {

class WindowPrivate;

class Window : public QMainWindow {
    Q_OBJECT
public:
    explicit Window(QWidget* parent = nullptr);
    ~Window() override;

private:
    QScopedPointer<WindowPrivate> p;
};

}  // namespace flipman