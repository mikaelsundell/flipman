// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#include "sdk.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTimer>
#include <flipmansdk/core/application.h>

namespace {
struct TestCase {
    const char* name;
    bool (*run)();
    const char* resource = nullptr;
};
using namespace flipman::sdk::test;
const TestCase cases[] = { { "file", testFile },
                           { "fps", testFps },
                           { "time", testTime },
                           { "time_range", testTimeRange },
                           { "smpte", testSmpte },
                           { "image_classification", testImageClassification },
                           { "image_nv12_layout", testImageNV12Layout },
                           { "image_packed_layout", testImagePackedLayout },
                           { "image_uint16", testImageUint16 },
                           { "image_double", testImageDouble },
                           { "image_planar", testImagePlanar },
                           { "image_interleaved", testImageInterleaved },
                           { "timeline", testTimeLine },
                           { "media_state", testMediaState },
                           { "shader_contract", testShaderContract },
                           { "clip", testClip, "quicktime/timecode export 29 97 04-05-03_10.mov" },
                           { "media", testMedia, "quicktime/23 967 fps 24 fps timecode.mp4" },
                           { "render_offscreen", testRenderOffscreen, "quicktime/23 967 fps 24 fps timecode.mp4" },
                           { "render_roundtrip", testRenderRoundtrip, "quicktime/23 967 fps 24 fps timecode.mp4" },
                           { "shader", testShader },
                           { "plugin_fx", testPluginFx },
                           { "plugin_container", testPluginContainer, "quicktime/23 967 fps 24 fps timecode.mp4" },
                           { "plugin_image", testPluginImage, "exr/test.00086400.exr" },
                           { "plugin_registry", testPluginRegistry, "quicktime/24fps.mov" },
                           { "image_nv12", testImageNV12, "quicktime/square export 23.976.mov" },
                           { "image_average", testImageAverage, "exr8k/8kdci long 23.976_00087172.exr" },
                           { "image_threaded", testImageThreaded, "exr8k" },
                           { "timer", testTimer } };
}  // namespace

int
main(int argc, char* argv[])
{
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--list") {
        for (const auto& test : cases)
            qInfo().noquote() << test.name;
        return 0;
    }
    if (argc < 2 || argc > 4) {
        qCritical() << "usage: flipmansdk_tests CASE [DATA_DIRECTORY] [--require-data]";
        return 2;
    }
    const QString name = QString::fromLocal8Bit(argv[1]);
    const TestCase* selected = nullptr;
    for (const auto& test : cases)
        if (name == test.name)
            selected = &test;
    if (!selected) {
        qCritical() << "unknown test:" << name;
        return 2;
    }
    const QString data = argc >= 3 ? QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath() : QString();
    if (selected->resource) {
        const QFileInfo resource(QDir(data).filePath(selected->resource));
        if (data.isEmpty() || !resource.exists()
            || (resource.isDir() && QDir(resource.filePath()).entryList(QDir::Files).isEmpty())) {
            qWarning() << "missing external test resource:" << selected->resource << "under" << data;
            return argc == 4 && QString::fromLocal8Bit(argv[3]) == "--require-data" ? 1 : 77;
        }
    }
    QTemporaryDir output(QDir::tempPath() + "/flipmansdk-test-XXXXXX");
    if (!output.isValid()) {
        qCritical() << "could not create test output directory";
        return 1;
    }
    flipman::sdk::core::Application app(argc, argv);
    QTimer::singleShot(0, [&]() {
        init(data, output.path());
        const bool ok = selected->run();
        qInfo() << name << (ok ? "PASSED" : "FAILED");
        app.exit(ok ? 0 : 1);
    });
    return app.exec();
}
