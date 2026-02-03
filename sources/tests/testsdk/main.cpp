// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "testsdk.h"

#include <flipmansdk/core/application.h>

#include <QDebug>
#include <QTimer>

namespace flipman::sdk::test {
template<typename Fn>
static bool
runTest(const char* name, Fn&& fn)
{
    qDebug().noquote() << QStringLiteral("testsuite: %1 ...").arg(name);
    const bool ok = fn();
    if (ok) {
        qDebug().noquote() << QStringLiteral("testsuite: %1 [OK]").arg(name);
    }
    else {
        qWarning().noquote() << QStringLiteral("testsuite: %1 [FAILED]").arg(name);
    }
    return ok;
}

bool
run()
{
    bool ok = true;
    const bool runContainers = false;
    const bool runTypes = false;
    const bool runMedia = false;
    const bool runTimer = false;
    const bool runPlugins = true;
    const bool runTimeLine = false;

    if (runContainers) {
        ok &= runTest("containers", [] { return testClip(); });
    }
    if (runTypes) {
        ok &= runTest("types", [] {
            return testFile() && testImage() && testTime() && testTimeRange() && testFps() && testSmpte();
        });
    }
    if (runMedia) {
        ok &= runTest("media", [] { return testMedia(); });
    }

    if (runTimer) {
        ok &= runTest("timer", [] { return testTimer(); });
    }

    if (runPlugins) {
        ok &= runTest("plugins", [] { return testPlugins() && testPluginRegistry(); });
    }

    if (runTimeLine) {
        ok &= runTest("timeline", [] { return testTimeLine(); });
    }

    qDebug().noquote() << (ok ? "testsuite: ALL TESTS PASSED" : "testsuite: TEST FAILURES DETECTED");

    return ok;
}
}  // namespace flipman::sdk::test

int
main(int argc, char* argv[])
{
    flipman::sdk::core::Application app(argc, argv);

    QTimer::singleShot(0, [&]() {
        const bool ok = flipman::sdk::test::run();
        QCoreApplication::exit(ok ? 0 : 1);
    });

    return app.exec();
}
