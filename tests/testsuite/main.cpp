// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "testsuite.h"

#include <QDebug>

int
main(int argc, char* argv[])
{
    bool test_types = true;
    bool test_containers = true;
    bool test_processors = true;
    bool test_plugins = true;

    if (test_containers) {
        qDebug() << "testsuite: containers";

        test_clip();
    }


    if (test_types) {
        qDebug() << "testsuite: types";


        //test_file();
        //test_image();
        test_time();
        //test_timerange();
        //test_fps();
        //test_smpte();
    }
    if (0) {
        test_media();
    }
    if (0) {
        test_timer();
    }
    if (1) {
        //test_plugins();
    }
    if (0) {
        test_pluginregistry();
    }
    if (0) {
        test_timeline();
    }
}
