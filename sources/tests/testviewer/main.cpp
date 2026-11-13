// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"

#include <flipmansdk/core/application.h>
#include <QApplication>
#include <QFileInfo>
#include <cstdio>

int
main(int argc, char* argv[])
{
    if (argc > 2 || (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--help")) {
        std::fprintf(stderr, "Usage: testviewer [media-file]\n");
        return argc == 2 ? 0 : 2;
    }
    if (argc == 2 && !QFileInfo(QString::fromLocal8Bit(argv[1])).isFile()) {
        std::fprintf(stderr, "testviewer: input file does not exist\n");
        return 2;
    }
    flipman::sdk::core::Application app(argc, argv);
    flipman::Window window;
    if (argc == 2 && !window.isValid())
        return 1;

    window.show();
    return app.exec();
}
