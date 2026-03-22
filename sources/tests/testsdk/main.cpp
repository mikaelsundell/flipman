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
    qDebug().noquote() << QStringLiteral("run: %1 ...").arg(name);
    const bool ok = fn();
    if (ok) {
        qDebug().noquote() << QStringLiteral("run: %1 [OK]").arg(name);
    }
    else {
        qWarning().noquote() << QStringLiteral("run: %1 [FAILED]").arg(name);
    }
    return ok;
}

bool
run()
{
    const bool runContainers = false;
    const bool runTypes = false;
    const bool runImage = false;
    const bool runMedia = false;
    const bool runTimer = false;
    const bool runPlugin = false;
    const bool runRender = true;
    const bool runShader = false;
    const bool runTimeLine = false;

    init();

    if (runContainers && !runTest("containers", [] { return testClip(); }))
        return false;

    if (runTypes
        && !runTest("types", [] { return testFile() && testTime() && testTimeRange() && testFps() && testSmpte(); }))
        return false;

    if (runImage && !runTest("image", [] { return testImage(); }))
        return false;

    if (runMedia && !runTest("media", [] { return testMedia(); }))
        return false;

    if (runTimer && !runTest("timer", [] { return testTimer(); }))
        return false;

    if (runPlugin && !runTest("plugin", [] { return testPlugin() && testPluginRegistry(); }))
        return false;

    if (runRender && !runTest("render", [] { return testRender(); }))
        return false;

    if (runShader && !runTest("shader", [] { return testShader(); }))
        return false;

    if (runTimeLine && !runTest("timeline", [] { return testTimeLine(); }))
        return false;

    qDebug().noquote() << "run: ALL TESTS PASSED";
    return true;
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
