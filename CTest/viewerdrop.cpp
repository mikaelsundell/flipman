// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "sources/tests/testviewer/window.h"

#include <flipmansdk/av/media.h>
#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/ui.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/widgets/timelineslider.h>
#include <flipmansdk/widgets/viewer.h>
#include <QApplication>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QImage>
#include <QLineEdit>
#include <QMenu>
#include <QMetaEnum>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QUrl>
#include <cmath>
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

static QAction*
profileAction(QMenu* menu, const QString& name)
{
    for (auto* action : menu->actions()) {
        if (!action->isSeparator() && action->data().toString() == name && !action->menu())
            return action;

        if (action->menu()) {
            if (auto* found = profileAction(action->menu(), name))
                return found;
        }
    }
    return nullptr;
}

static void
selectProfile(QPushButton* button, const QString& name)
{
    auto* action = profileAction(button->menu(), name);
    require(action, "profile missing from grouped menu");
    action->trigger();
}

int
main(int argc, char** argv)
{
    flipman::sdk::core::Application app(argc, argv);
    try {
        auto* theme = app.ui();
        require(theme && theme == flipman::sdk::core::ui(), "application UI service differs from global UI");
        require(!app.styleSheet().isEmpty() && !app.styleSheet().contains(QRegularExpression("\\$[a-zA-Z]")),
                "stylesheet has unresolved tokens");
        const auto icons = QMetaEnum::fromType<flipman::sdk::core::UI::IconRole>();
        for (int i = 0; i < icons.keyCount(); ++i) {
            const auto role = static_cast<flipman::sdk::core::UI::IconRole>(icons.value(i));
            require(!theme->icon(role).isNull(), icons.key(i));
        }
        const auto originalFormat = QSurfaceFormat::defaultFormat();
        const auto originalColorSpace = theme->colorSpace();
        auto format = originalFormat;
        format.setSamples(4);
        QSurfaceFormat::setDefaultFormat(format);
        theme->setColorSpace(flipman::sdk::render::DisplayColorSpace(QColorSpace(QColorSpace::DisplayP3)));
        require(theme->colorSpace().isValid() && !theme->colorSpace().iccProfile().isEmpty(),
                "display color-space facade lost its profile");
        require(QSurfaceFormat::defaultFormat().colorSpace() == QColorSpace(QColorSpace::DisplayP3)
                    && QSurfaceFormat::defaultFormat().samples() == 4,
                "default surface format options were overwritten");
        theme->setColorSpace(originalColorSpace);
        QSurfaceFormat::setDefaultFormat(originalFormat);
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
        auto* slider = window.findChild<flipman::sdk::widgets::TimelineSlider*>();
        require(viewer && slider, "viewer controls missing");
        auto* backgroundMenu = window.findChild<QMenu*>("timelineBackgroundMenu");
        require(backgroundMenu && viewer->renderEngine()->background() == QColor(Qt::black),
                "viewer background menu/default missing");
        const auto colorSettings = viewer->renderEngine()->colorSettings();
        for (const auto& name : { QStringLiteral("White"), QStringLiteral("Black") }) {
            QAction* preset = nullptr;
            for (auto* action : backgroundMenu->actions())
                if (action->text() == name)
                    preset = action;
            require(preset, "background preset missing");
            preset->trigger();
            require(viewer->renderEngine()->background() == QColor(name.toLower()), "background preset was ignored");
            require(viewer->renderEngine()->colorSettings() == colorSettings, "background changed the color transform");
        }
        const auto displayBefore = viewer->displayTransform();
        const auto profileBefore = viewer->displayColorSpace();
        theme->setColorSpace(flipman::sdk::render::DisplayColorSpace(QColorSpace(QColorSpace::DisplayP3)));
        require(viewer->displayTransform().colorSpace == displayBefore.colorSpace
                    && viewer->displayTransform().transferFunction == displayBefore.transferFunction,
                "default surface color space overwrote the viewer Display tag");
        require(viewer->displayColorSpace() == profileBefore, "UI default replaced the viewer output profile");
        theme->setColorSpace(originalColorSpace);
        auto* output = window.findChild<QPushButton*>("output");
        auto* view = window.findChild<QComboBox*>("view");
        auto* metalTag = window.findChild<QComboBox*>("metalTag");
        require(output && view && metalTag, "color controls missing");
        auto* input = window.findChild<QPushButton*>("input");
        auto* working = window.findChild<QPushButton*>("working");
        auto* colorControls = window.findChild<QWidget*>("ColorControls");
        require(input && input->menu() && working && colorControls && !window.findChild<QSplitter*>(),
                "compact color groups/spacer missing");
        QMenu* cameraMenu = nullptr;
        for (auto* action : input->menu()->actions()) {
            if (action->text() == "Input")
                cameraMenu = action->menu();
        }
        require(cameraMenu && !cameraMenu->actions().isEmpty(), "input camera families are not grouped");
        auto* raw = profileAction(input->menu(), "Raw");
        require(raw && input->menu()->actions().at(1)->isSeparator(), "Raw/Auto divider missing");
        selectProfile(input, "Raw");
        selectProfile(input, QString());
        selectProfile(working, "ARRI LogC3 (EI800)");
        auto* workingTransfer = window.findChild<QComboBox*>("workingTransfer");
        require(workingTransfer && workingTransfer->findText("Linear") >= 0, "ARRI linear encoding option missing");
        workingTransfer->setCurrentText("Linear");
        require(viewer->renderEngine()->colorSettings().workingSpace == "Linear ARRI Wide Gamut 3",
                "gamut/encoding pair did not resolve to the exact OCIO space");
        workingTransfer->setCurrentText("LogC3 (EI800)");
        require(viewer->renderEngine()->colorSettings().workingSpace == "ARRI LogC3 (EI800)",
                "working selector ignored log space");
        workingTransfer->setCurrentText("Gamma 2.4");
        require(viewer->renderEngine()->colorSettings().workingSpace
                    == flipman::sdk::render::ColorPipeline::combinedColorSpace("ARRI Wide Gamut 3", "Gamma 2.4"),
                "independent working gamma was ignored");
        selectProfile(working, "Raw");
        selectProfile(working, QString());
        selectProfile(working, "ACEScg");
        for (auto* button : { input, working, output }) {
            const auto actions = button->menu()->actions();
            require(actions.front()->text() == "Auto" && actions.at(1)->isSeparator() && actions.last()->text() == "Raw"
                        && actions.at(actions.size() - 2)->isSeparator(),
                    "color menu ordering differs from Auto / options / Raw");
        }
        require(view->itemText(view->count() - 1) == "Raw" && view->itemText(view->count() - 2).isEmpty(),
                "View Raw divider missing");
        selectProfile(output, "Raw");
        require(viewer->renderEngine()->colorSettings().display == "Raw", "output bypass was ignored");
        require(workingTransfer->itemText(0) == "Auto: Linear", "Auto gamma does not show its resolved curve");
        selectProfile(output, "Display P3 - Display");
        require(viewer->displayTransform().colorSpace == flipman::sdk::render::ColorSpace::DisplayP3,
                "matching output did not sync Metal tag");
        selectProfile(output, "Rec.2100-PQ - Display");
        require(viewer->displayTransform().colorSpace == flipman::sdk::render::ColorSpace::DisplayP3,
                "unmatched output overwrote Metal tag");
        selectProfile(output, "Rec.1886 Rec.709 - Display");
        metalTag->setCurrentText("Rec.709 Gamma 2.6");
        view->setCurrentText("ACES 2.0 - SDR 100 nits (Rec.709)");
        require(viewer->displayTransform().transferFunction == flipman::sdk::render::TransferFunction::Gamma26,
                "view change overwrote manual Metal tag");
        view->setCurrentText("No tone mapping");
        require(viewer->renderEngine()->colorSettings().view == "Un-tone-mapped",
                "friendly view label changed the OCIO identifier");
        require(drop(viewer, { QUrl::fromLocalFile(first) }), "first image drop failed");
        require(window.isValid() && window.windowTitle().contains("first.png"), "first image was not loaded");
        require(window.findChild<QLineEdit*>("smpteField")->text() == "00:00:00:00",
                "first image timestamp followed the next reader cursor");
        require(drop(&window, { QUrl::fromLocalFile(second) }), "replacement drop failed");
        require(window.windowTitle().contains("second.png") && slider->value() == 1 && slider->maximum() == 1,
                "replacement did not reset title/timeline");
        const auto movie = qEnvironmentVariable("FLIPMAN_VIEWER_TEST_MOVIE");
        if (!movie.isEmpty()) {
            require(drop(viewer, { QUrl::fromLocalFile(movie) }), "movie drop failed");
            require(slider->maximum() > 1, "movie did not initialize timeline");
            auto* frame = window.findChild<QSpinBox*>("frameField");
            auto* total = window.findChild<QLineEdit*>("totalFramesField");
            auto* smpte = window.findChild<QLineEdit*>("smpteField");
            require(frame && total && smpte, "timeline fields missing");
            flipman::sdk::av::Media source;
            require(source.open(flipman::sdk::core::File(movie)) && source.waitForOpened(5000),
                    "timecode reference open failed");
            const QString metadataTitle = source.metaData().value("title").toString().trimmed();
            if (!metadataTitle.isEmpty())
                require(window.windowTitle() == "testviewer — " + metadataTitle, "movie metadata title was ignored");
            QString expected = flipman::sdk::av::SmpteTime(source.start()).toString();
            if (source.fps().dropFrame())
                expected.replace('.', ';');
            require(smpte->text() == expected, "source SMPTE start was replaced by relative timeline time");
            const auto suppliedTimecode = qEnvironmentVariable("FLIPMAN_VIEWER_EXPECTED_TIMECODE");
            if (!suppliedTimecode.isEmpty())
                require(smpte->text() == suppliedTimecode, "source SMPTE differs from supplied reference");
            auto* resolution = window.findChild<QComboBox*>("projectResolution");
            resolution->setCurrentIndex(resolution->findData(QSize(1920, 1080)));
            require(viewer->renderEngine()->resolution() == QSize(1920, 1080), "project resolution selector ignored");
            resolution->setCurrentIndex(0);
            if (QGuiApplication::platformName() == "cocoa") {
                window.show();
                app.processEvents();
                const auto framebuffer = viewer->grabFramebuffer();
                require(!framebuffer.isNull(), "native Metal viewer failed to render");
                const auto size = viewer->renderEngine()->resolution();
                const float expectedZoom = std::min(float(framebuffer.width()) / size.width(),
                                                    float(framebuffer.height()) / size.height());
                require(std::abs(viewer->zoom() - expectedZoom) < 0.001f, "native Fit differs from project bounds");
                const auto originalTag = viewer->displayTransform();
                viewer->setDisplayTransform(
                    { flipman::sdk::render::ColorSpace::DisplayP3, flipman::sdk::render::TransferFunction::SRGB });
                require(viewer->displayColorSpace().qtColorSpace() == QColorSpace(QColorSpace::DisplayP3),
                        "native output profile did not update");
                require(!viewer->grabFramebuffer().isNull(), "Metal profile replacement failed");
                window.resize(window.width() + 32, window.height() + 16);
                app.processEvents();
                require(!viewer->grabFramebuffer().isNull(), "Metal profile reuse after resize failed");
                viewer->setDisplayTransform(originalTag);
                const auto snapshot = qEnvironmentVariable("FLIPMAN_VIEWER_NATIVE_SNAPSHOT");
                if (!snapshot.isEmpty()) {
                    require(framebuffer.save(snapshot + ".frame.png"), "could not capture Metal framebuffer");
                    require(window.centralWidget()->grab().save(snapshot), "could not capture native viewer layout");
                }
            }
            frame->setValue(2);
            const auto start = source.start();
            expected
                = flipman::sdk::av::SmpteTime(flipman::sdk::av::Time(start, start.ticks() + start.ticks(1))).toString();
            if (source.fps().dropFrame())
                expected.replace('.', ';');
            require(smpte->text() == expected, "source SMPTE did not advance after seek");
            require(slider->value() == 2 && total->text().toInt() == slider->maximum(), "frame field did not seek");
            require(smpte->text().count(':') + smpte->text().count(';') == 3, "SMPTE field is not a full timecode");

            // The imported ruler seeks to the clicked frame rather than stepping by a page.
            slider->resize(500, 40);
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(490, 20), QPointF(490, 20), Qt::LeftButton,
                              Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(slider, &press);
            QMouseEvent release(QEvent::MouseButtonRelease, QPointF(490, 20), QPointF(490, 20), Qt::LeftButton,
                                Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(slider, &release);
            require(slider->value() > slider->maximum() / 2 && frame->value() == slider->value(),
                    "timeline ruler did not scrub to the clicked frame");
            require(drop(viewer, { QUrl::fromLocalFile(second) }), "image drop after movie failed");
            require(slider->value() == 1 && slider->maximum() == 1, "image retained movie timeline");
        }
        require(!drop(viewer, { QUrl("https://example.com/movie.mov") }), "remote URL accepted");
        require(!drop(viewer, { QUrl::fromLocalFile(dir.path()) }), "directory accepted");
        require(!drop(viewer, { QUrl::fromLocalFile(first), QUrl::fromLocalFile(second) }), "multiple files accepted");
        require(!drop(viewer, { QUrl::fromLocalFile(broken) }), "invalid image accepted");
        require(window.isValid() && window.windowTitle().contains("second.png"), "failed drop replaced media");
        require(!window.statusBar()->currentMessage().isEmpty(), "failed drop has no diagnostic");
        require(drop(viewer, { QUrl::fromLocalFile(first) }), "reload after failure failed");
        require(!window.statusBar()->currentMessage().contains("Could not"), "reload retained stale diagnostic");
        const auto snapshot = qEnvironmentVariable("FLIPMAN_VIEWER_CONTROLS_SNAPSHOT");
        if (!snapshot.isEmpty()) {
            window.show();
            app.processEvents();
            require(window.centralWidget()->grab().save(snapshot), "could not capture viewer layout");
        }
        return 0;
    } catch (const std::exception& error) {
        qCritical("%s", error.what());
        return 1;
    }
}
