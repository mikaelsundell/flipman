// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "testsuite.h"

#include <QDebug>

int
main(int argc, char* argv[])
{
    bool test_containers = true;
    bool test_types = true;
    bool test_media = true;
    bool test_timer = true;
    bool test_plugins = true;
    bool test_processors = true;

    if (test_containers) {
        qDebug() << "testsuite: containers";
        test_clip();
    }

    if (test_types) {
        qDebug() << "testsuite: types";
        test_file();
        test_image();
        test_time();
        test_timerange();
        test_fps();
        test_smpte();
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
        test_pluginregistry();
    }
    if (test_timeline) {
        test_timeline();
    }
}
