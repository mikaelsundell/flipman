// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "testsuite.h"

#include <flipmansdk/core/application.h>

#include <QTimer>
#include <QDebug>

void
run()
{
    bool test_containers = true;
    bool test_types = true;
    bool test_media = true;
    bool test_timer = true;
    bool test_plugins = true;
    bool test_processors = true;
    bool test_timeline = true;

    if (test_containers) {
        qDebug() << "testsuite: containers";
        flipman::test_clip();
    }

    if (test_types) {
        qDebug() << "testsuite: types";
        flipman::test_file();
        flipman::test_image();
        flipman::test_time();
        flipman::test_timerange();
        flipman::test_fps();
        flipman::test_smpte();
    }
    if (test_media) {
        qDebug() << "testsuite: media";
        //test_media();
    }
    if (test_timer) {
        qDebug() << "testsuite: timer";
        //test_timer();
    }
    if (test_plugins) {
        qDebug() << "testsuite: timer";
        //test_plugins();
        flipman::test_pluginregistry();
    }
    if (test_timeline) {
        flipman::test_timeline();
    }
}

int
main(int argc, char* argv[])
{
    flipman::sdk::core::Application app(argc, argv);
    QTimer::singleShot(0, [&]() {
        run();
        app.quit();
    });
    return app.exec();
}
