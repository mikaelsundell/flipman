// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"
#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Window window;
    window.show();
    return app.exec();
}
