// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell
#include "sources/tests/testviewer/window.h"
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QImage>
#include <QMimeData>
#include <QSlider>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QUrl>
#include <flipmansdk/core/application.h>
#include <flipmansdk/widgets/viewer.h>
#include <stdexcept>

static void
require(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

static bool
drop(QWidget* target, const QList<QUrl>& urls)
{
    QMimeData mime;
    mime.setUrls(urls);
    QDragEnterEvent enter(QPoint(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &enter);
    if (!enter.isAccepted())
        return false;
    QDropEvent event(QPointF(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &event);
    return event.isAccepted();
}

int
main(int argc, char** argv)
{
    flipman::sdk::core::Application app(argc, argv);
    try {
        QTemporaryDir dir;
        require(dir.isValid(), "temporary directory unavailable");
        const auto first = dir.filePath("first.png");
        const auto second = dir.filePath("second.png");
        const auto broken = dir.filePath("broken.png");
        QImage image(16, 8, QImage::Format_RGBA8888);
        image.fill(Qt::red);
        require(image.save(first), "could not create first fixture");
        image = QImage(8, 16, QImage::Format_RGBA8888);
        image.fill(Qt::blue);
        require(image.save(second), "could not create second fixture");
        QFile invalid(broken);
        require(invalid.open(QIODevice::WriteOnly), "could not create broken fixture");
        invalid.write("invalid image");
        invalid.close();

        flipman::Window window;
        require(!window.isValid(), "empty viewer unexpectedly has media");
        auto* viewer = window.findChild<flipman::sdk::widgets::Viewer*>();
        auto* slider = window.findChild<QSlider*>();
        require(viewer && slider, "viewer controls missing");
        require(drop(viewer, { QUrl::fromLocalFile(first) }), "first image drop failed");
        require(window.isValid() && window.windowTitle().contains("first.png"), "first image was not loaded");
        require(drop(&window, { QUrl::fromLocalFile(second) }), "replacement drop failed");
        require(window.windowTitle().contains("second.png") && slider->value() == 0 && slider->maximum() == 0,
                "replacement did not reset title/timeline");
        const auto movie = qEnvironmentVariable("FLIPMAN_VIEWER_TEST_MOVIE");
        if (!movie.isEmpty()) {
            require(drop(viewer, { QUrl::fromLocalFile(movie) }), "movie drop failed");
            require(slider->maximum() > 1, "movie did not initialize timeline");
            slider->setValue(1);
            require(drop(viewer, { QUrl::fromLocalFile(second) }), "image drop after movie failed");
            require(slider->value() == 0 && slider->maximum() == 0, "image retained movie timeline");
        }
        require(!drop(viewer, { QUrl("https://example.com/movie.mov") }), "remote URL accepted");
        require(!drop(viewer, { QUrl::fromLocalFile(dir.path()) }), "directory accepted");
        require(!drop(viewer, { QUrl::fromLocalFile(first), QUrl::fromLocalFile(second) }), "multiple files accepted");
        require(!drop(viewer, { QUrl::fromLocalFile(broken) }), "invalid image accepted");
        require(window.isValid() && window.windowTitle().contains("second.png"), "failed drop replaced media");
        require(!window.statusBar()->currentMessage().isEmpty(), "failed drop has no diagnostic");
        require(drop(viewer, { QUrl::fromLocalFile(first) }), "reload after failure failed");
        require(window.statusBar()->currentMessage().isEmpty(), "reload retained stale diagnostic");
        return 0;
    } catch (const std::exception& error) {
        qCritical("%s", error.what());
        return 1;
    }
}
