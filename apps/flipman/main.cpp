// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "flipman.h"
#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Flipman flipman;
    flipman.set_arguments(QCoreApplication::arguments());
    flipman.show();
    return app.exec();
}
