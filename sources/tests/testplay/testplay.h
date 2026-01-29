// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include <QWidget>

class Testplay : public QWidget {
    Q_OBJECT
public:
    Testplay(QWidget* parent = nullptr);
    virtual ~Testplay();
};
