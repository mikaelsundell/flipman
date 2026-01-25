// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "testplay.h"

#include <QApplication>

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Testplay testplay;
    testplay.show();
    return app.exec();
}
