// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "app/viewer.h"
#include "commands/commanddisplay.h"
#include "commands/commandtimeline.h"
#include "core/commandstack.h"
#include "core/console.h"
#include "core/session.h"
#include "core/settings.h"
#include "mcp/mcpserver.h"
#include "python/pythoninterpreter.h"
#include "views/renderview.h"

#include <flipmansdk/av/clip.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/environment.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/plugins/quicktime/quicktimewriter.h>
#include <QApplication>
#include <QColorSpace>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScopeGuard>
#include <QScreen>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTemporaryDir>
#include <QThread>
#include <QTreeWidget>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>

using namespace flipman;
using namespace flipman::sdk;

namespace {

void
require(bool value, const QString& message)
{
    if (!value)
        throw std::runtime_error(message.toStdString());
}

void
waitFor(const std::function<bool()>& ready)
{
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < 15000) {
        QApplication::processEvents();
        QThread::msleep(2);
    }
    require(ready(), "timed out waiting for application state");
}

QString
movie(const QString& path)
{
    plugins::QuicktimeWriter writer;
    writer.setFps(av::Fps::fps24());
    writer.setTimeRange({ av::Time::zero(av::Fps::fps24()), av::Time::fromFrames(24, av::Fps::fps24()) });
    core::MetaData metadata;
    metadata.insert("title", "Application fixture");
    writer.setMetaData(metadata);
    plugins::MediaWriter::Options options;
    options.values.insert("codec", "prores422");
    require(writer.open(core::File(path), options), writer.error().message());
    for (int i = 0; i < 24; ++i) {
        core::ImageBuffer image(QRect(0, 0, 128, 64), QRect(0, 0, 128, 64), core::ImageFormat::UInt8, 4);
        image.setPixelLayout(core::ImageBuffer::PixelLayout::BGRA);
        image.allocate();
        memset(image.data(), i * 8, image.byteSize());
        require(writer.write(image).isValid(), writer.error().message());
    }
    require(writer.close(), writer.error().message());
    return path;
}

void
load(Session* session, const QString& path)
{
    session->openFile(path);
    waitFor([&]() { return !session->isLoading(); });
    require(session->isLoaded() && session->filename() == path, session->status());
}

void
model(Session* session, const QString& path, const QString& directory)
{
    load(session, path);
    require(session->frameCount() == 24 && session->title() == "Application fixture", "movie metadata not reflected");
    auto* timeline = session->timeline();
    require(timeline->tracks().size() == 1, "session must own one track");
    auto* track = timeline->tracks().first();
    require(track->clips().size() == 1 && track->clips().first()->media().isOpen(), "missing clip/media");
    require(track->clipRange(track->clips().first()).duration().frames() == 24, "wrong clip range");
    require(session->setFrame(10), "seek rejected");
    waitFor([&]() { return session->presentedFrame() == 10; });
    require(std::abs(int(session->image().data()[0]) - 80) < 6, "forward seek decoded the wrong frame");
    require(session->setFrame(2), "backward seek rejected");
    waitFor([&]() { return session->presentedFrame() == 2; });
    require(std::abs(int(session->image().data()[0]) - 16) < 6, "backward seek decoded the wrong frame");
    const QString invalid = directory + "/invalid.mov";
    QFile file(invalid);
    require(file.open(QIODevice::WriteOnly), "fixture open failed");
    file.write("invalid movie");
    file.close();
    session->openFile(invalid);
    waitFor([&]() { return !session->isLoading(); });
    require(session->timeline() == timeline && session->isLoaded(), "failed open replaced current document");
    const QString still = directory + "/still.png";
    QImage image(80, 40, QImage::Format_RGB32);
    image.fill(Qt::red);
    require(image.save(still), "PNG fixture failed");
    session->openFile(still);
    session->openFile(invalid);
    waitFor([&]() { return !session->isLoading(); });
    require(session->timeline() == timeline, "superseded open replaced timeline");
    require(session->setFrame(12), "seek after failed replacement rejected");
    waitFor([&]() { return session->presentedFrame() == 12; });
    require(session->image().dataWindow().width() == 128, "superseded worker replaced active reader");
    load(session, still);
    require(session->frameCount() == 1 && session->image().dataWindow().width() == 80, "image replacement failed");
    load(session, path);
    session->setEveryFrame(true);
    session->togglePlaying();
    waitFor([&]() { return session->presentedFrame() >= 2; });
    session->timeline()->stop();
    require(!session->timeline()->isPlaying(), "stop failed");
    session->setFrame(0);
    session->setFps(av::Fps(1000, 1));
    session->setEveryFrame(false);
    session->togglePlaying();
    waitFor([&]() { return !session->timeline()->isPlaying(); });
    waitFor([&]() { return session->presentedFrame() == 23; });
    require(session->timeline()->time().frames() == 23, "deadline playback did not finish on the final frame");
    session->setFrame(20);
    session->closeFile();
    QApplication::processEvents();
    require(!session->isLoaded() && session->timeline()->tracks().isEmpty(), "close retained media");
    auto transient = std::make_unique<Session>();
    transient->openFile(path);
    transient->closeFile();
    transient.reset();

    // Exercise destruction and reuse while AVFoundation still owns open callbacks.
    std::unique_ptr<plugins::MediaReader> reader(core::pluginRegistry()->getPlugin<plugins::MediaReader>("mov"));
    require(bool(reader), "QuickTime reader unavailable");
    for (int i = 0; i < 32; ++i) {
        require(reader->open(core::File(path), {}), "rapid open rejected");
        if (i % 2 == 0)
            QApplication::processEvents();
        reader->close();
    }
    for (int i = 0; i < 8; ++i) {
        std::unique_ptr<plugins::MediaReader> temporary(core::pluginRegistry()->getPlugin<plugins::MediaReader>("mov"));
        require(temporary && temporary->open(core::File(path), {}), "temporary open rejected");
    }
}

void
commands(Session* session, const QString& path)
{
    load(session, path);
    auto* stack = session->commands();
    stack->execute(timeCommand(av::Time::zero(av::Fps::fps24()), av::Time::fromFrames(7, av::Fps::fps24())));
    require(session->timeline()->time().frames() == 7 && stack->canUndo(), "time command failed");
    stack->undo();
    require(session->timeline()->time().frames() == 0, "time undo failed");
    stack->redo();
    require(session->timeline()->time().frames() == 7, "time redo failed");
    stack->execute(fpsCommand(session->timeline()->fps(), av::Fps::fps30()));
    require(session->timeline()->fps() == av::Fps::fps30() && session->frameCount() == 24, "FPS changed frame count");
    stack->undo();
    require(session->timeline()->fps() == av::Fps::fps24(), "FPS undo failed");
    const auto before = session->displayTransform();
    stack->execute(displayCommand(before, { render::ColorSpace::DisplayP3, render::TransferFunction::SRGB }));
    require(session->displayTransform().colorSpace == render::ColorSpace::DisplayP3, "display command failed");
    stack->undo();
    require(session->displayTransform().colorSpace == before.colorSpace, "display undo failed");
    auto settings = session->colorSettings();
    settings.workingSpace = "Raw";
    require(session->setColorSettings(settings, {}), "working bypass failed");
    require(session->colorSettings().workingSpace == "Raw", "working bypass selection lost");
    session->closeFile();
    require(!stack->canUndo(), "history crossed document boundary");
}

void
python(Session* session, const QString& path)
{
    load(session, path);
    auto* interpreter = flipman::pythonInterpreter();
    auto output = interpreter->executeScript(
        "print(session.filename())\nprint(session.timeline().fps().real())\nprint(len(session.timeline().tracks()))\nanswer=42");
    require(output.contains(path) && output.contains("24.0") && !output.contains("Traceback"), output);
    output = interpreter->executeScript(
        "session.setFrame(5)\nsession.setFps(sdk.Fps(30,1))\nprint(answer)\nprint(session.timeline().time().frames())");
    require(output.contains("42") && output.contains("5") && session->timeline()->fps() == av::Fps::fps30(), output);
    session->commands()->undo();
    require(session->timeline()->fps() == av::Fps::fps24(), "Python used a different command stack");
    output = interpreter->executeScript(
        "import threading, time\nfinished=threading.Event()\ndef work():\n    time.sleep(0.01)\n    finished.set()\nthreading.Thread(target=work).start()");
    require(!output.contains("Traceback"), output);
    QThread::msleep(100);
    output = interpreter->executeScript("print(finished.is_set())");
    require(output.contains("True"), "Embedded interpreter retained the GIL between scripts");
    output = interpreter->executeScript("session.setFrame(9999)");
    require(output.contains("ValueError"), output);
    output = interpreter->executeScript("session.setDisplayProfile('sRGB')\nprint('alive')");
    require(output.contains("alive") && session->displayTransform().transferFunction == render::TransferFunction::SRGB,
            output);
}

std::atomic<int> unreleasedResources = 0;

void
viewLifetime(flipman::Application& application, const QString& path)
{
    load(application.session(), path);
    auto* engine = application.session()->renderEngine();
    const auto previousHandler = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext&, const QString& message) {
            if (message.contains("unreleased resources"))
                ++unreleasedResources;
        });
    const auto restoreHandler = qScopeGuard([previousHandler]() { qInstallMessageHandler(previousHandler); });
    for (int cycle = 0; cycle < 2; ++cycle) {
        auto view = std::make_unique<RenderView>();
        view->resize(800, 500);
        view->show();
        waitFor([&]() { return engine->initialized(); });
        require(engine->imageLayers().size() == 1, "viewport recreation lost source image");
        view.reset();
        require(!engine->initialized(), "viewport destruction left GPU resources alive");
        require(engine->imageLayers().size() == 1 && application.session()->isLoaded(),
                "GPU release cleared document state");
    }
    require(unreleasedResources == 0, "QRhi was destroyed with live engine resources");
}

struct Response {
    int status = 0;
    QJsonObject body;
};

Response
request(QNetworkAccessManager& network, const QJsonObject& message, const QByteArray& origin = {},
        const QByteArray& host = {})
{
    QNetworkRequest request(flipman::mcpServer()->url());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!origin.isEmpty())
        request.setRawHeader("Origin", origin);
    if (!host.isEmpty())
        request.setRawHeader("Host", host);
    auto* reply = network.post(request, QJsonDocument(message).toJson(QJsonDocument::Compact));
    waitFor([&]() { return reply->isFinished(); });
    Response result { reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                      QJsonDocument::fromJson(reply->readAll()).object() };
    reply->deleteLater();
    return result;
}

QJsonObject
call(QNetworkAccessManager& network, const QString& name, const QJsonObject& arguments = {})
{
    auto response = request(network, { { "jsonrpc", "2.0" },
                                       { "id", 1 },
                                       { "method", "tools/call" },
                                       { "params", QJsonObject { { "name", name }, { "arguments", arguments } } } });
    require(response.status == 200, "MCP tool transport failed");
    return response.body.value("result").toObject();
}

QString
content(const QJsonObject& result)
{
    return result.value("content").toArray().first().toObject().value("text").toString();
}

void
services(flipman::Application& application)
{
    require(flipman::app() == &application && flipman::session() == application.session(),
            "global application facade diverged");
    require(application.settings()->isIsolated() && !application.settingsDirectory().isEmpty(),
            "test used normal preferences");
    require(application.settings()->filename().startsWith(application.settingsDirectory()),
            "settings escaped clean directory");
    application.settings()->setValue("test/marker", "isolated");
    application.settings()->sync();
    require(application.settings()->value("test/marker").toString() == "isolated", "settings facade failed");
    Settings other(application.settingsDirectory() + "/other");
    require(other.value("test/marker", "absent") == "absent", "isolated settings read another process's preferences");
    require(application.console() && !application.console()->isRunning(), "console capture should be opt-in");
    require(application.console()->start(), "console did not start");
    std::fprintf(stderr, "native console fixture\n");
    std::fflush(stderr);
    waitFor([&]() { return application.console()->text().contains("native console fixture"); });
    application.console()->stop();
    require(!application.console()->isRunning(), "console did not stop");
}

void
mcp(flipman::Application& application, const QString& path)
{
    Viewer viewer;
    load(application.session(), path);
    require(!application.mcpServer()->isRunning(), "MCP must be opt-in");
    qputenv("FLIPMAN_TEST_MCP_PORT", "0");
    require(application.mcpServer()->start(), application.mcpServer()->errorString());
    QNetworkAccessManager network;
    auto initialized = request(network, { { "jsonrpc", "2.0" }, { "id", 0 }, { "method", "initialize" } });
    require(initialized.status == 200
                && initialized.body.value("result").toObject().value("serverInfo").toObject().value("name")
                       == "Flipman",
            "MCP initialization failed");
    auto list = request(network, { { "jsonrpc", "2.0" }, { "id", 1 }, { "method", "tools/list" } });
    require(list.body.value("result").toObject().value("tools").toArray().size() == 13,
            "MCP tool registration incomplete");
    require(request(network, { { "jsonrpc", "2.0" }, { "method", "notifications/initialized" } }).status == 202,
            "MCP notification response failed");
    require(request(network, { { "jsonrpc", "2.0" }, { "id", 1 }, { "method", "ping" } }, "https://untrusted.invalid")
                    .status
                == 403,
            "MCP accepted a foreign browser origin");
    require(request(network, { { "jsonrpc", "2.0" }, { "id", 1 }, { "method", "ping" } }, {}, "foreign.invalid").status
                == 403,
            "MCP accepted a foreign host");
    auto state = QJsonDocument::fromJson(content(call(network, "flipman_get_session_info")).toUtf8()).object();
    require(state.value("loaded").toBool() && state.value("filename") == path, "MCP returned a different session");
    require(!call(network, "flipman_set_time", { { "frame", 7 } }).value("isError").toBool(), "MCP seek failed");
    require(application.session()->timeline()->time().frames() == 7
                && viewer.findChild<QSpinBox*>("frameField")->value() == 8,
            "MCP did not update views through Session");
    call(network, "flipman_set_fps", { { "numerator", 25 } });
    require(application.session()->timeline()->fps() == av::Fps::fps25(), "MCP FPS failed");
    call(network, "flipman_undo");
    require(application.session()->timeline()->fps() == av::Fps::fps24(), "MCP used a different undo stack");
    call(network, "flipman_set_display_profile", { { "name", "Display P3 Gamma 2.4" } });
    require(viewer.findChild<QComboBox*>("display")->currentText() == "Display P3 Gamma 2.4",
            "MCP display change missed UI");
    require(call(network, "flipman_set_time", { { "frame", -1 } }).value("isError").toBool(),
            "MCP accepted an invalid frame");
    require(call(network, "flipman_set_fps", { { "numerator", 1.5 } }).value("isError").toBool(),
            "MCP accepted fractional integer input");
    require(call(network, "flipman_set_time", { { "frame", 1 }, { "surprise", true } }).value("isError").toBool(),
            "MCP ignored unknown arguments");
    require(call(network, "unknown_tool").value("isError").toBool(), "MCP ignored an unknown tool");
    auto output
        = call(network, "flipman_run_python",
               { { "code", "print(session.timeline().time().frames())\nprint(app.applicationInfo()['isolated'])" } });
    require(content(output).contains("7") && content(output).contains("True"),
            "MCP Python did not share application services");
    call(network, "flipman_stop");
    call(network, "flipman_close_file");
    require(!application.session()->isLoaded(), "MCP close did not remove the source");
    require(!call(network, "flipman_open_file", { { "filename", path } }).value("isError").toBool(),
            "MCP open rejected fixture");
    waitFor([&]() { return !application.session()->isLoading(); });
    require(application.session()->isLoaded() && application.session()->timeline()->tracks().size() == 1,
            "MCP open failed to create composition");
    call(network, "flipman_play");
    require(application.session()->timeline()->isPlaying(), "MCP did not start playback");
    waitFor([&]() { return application.session()->presentedFrame() > 0; });
    call(network, "flipman_stop");
    require(!application.session()->timeline()->isPlaying(), "MCP did not stop playback");
    application.mcpServer()->stop();
    require(!application.mcpServer()->isRunning(), "MCP shutdown failed");
    qputenv("FLIPMAN_TEST_MCP_PORT", "invalid");
    require(!application.mcpServer()->start() && !application.mcpServer()->errorString().isEmpty(),
            "MCP accepted invalid port");
    qunsetenv("FLIPMAN_TEST_MCP_PORT");
}

}  // namespace

int
main(int argc, char** argv)
{
    QTemporaryDir directory;
    qputenv("FLIPMAN_TEST_SETTINGS_DIR", directory.path().toUtf8());
    flipman::Application application(argc, argv);
    try {
        require(directory.isValid(), "temporary directory failed");
        const auto path = movie(directory.filePath("fixture.mov"));
        const QString mode = argc > 1 ? argv[1] : "session";
        if (mode == "viewer") {
            Viewer viewer;
            viewer.show();
            require(viewer.session() == application.session(), "viewer bypassed application session");
            const auto external = qEnvironmentVariable("FLIPMAN_APP_VIEWER_FILE");
            load(viewer.session(), external.isEmpty() ? path : external);
            if (external.isEmpty())
                require(viewer.windowTitle().contains("Application fixture"), "viewer title ignored metadata");
            auto* splitter = viewer.findChild<QSplitter*>();
            require(splitter && splitter->count() == 3 && splitter->isCollapsible(0),
                    "panels must collapse by dragging");
            splitter->setSizes({ 0, 1000, 0 });
            QApplication::processEvents();
            require(splitter->sizes().first() == 0 && splitter->sizes().last() == 0, "panel collapse failed");
            splitter->setSizes({ 240, 1000, 260 });
            require(viewer.findChild<QTreeWidget*>()->topLevelItemCount() > 0, "metadata view empty");
            if (!qEnvironmentVariable("FLIPMAN_APP_SCREENSHOT").isEmpty()) {
                QElapsedTimer clock;
                clock.start();
                while (clock.elapsed() < 1000) {
                    QApplication::processEvents();
                    QThread::msleep(5);
                }
                auto capture = viewer.screen()->grabWindow(viewer.winId()).toImage();
                if (capture.isNull())
                    capture = viewer.grab().toImage();

                // Test capture only: omit ICC embedding to avoid the dependency prefix's libpng ICC writer bug.
                capture.setColorSpace(QColorSpace());
                require(capture.save(qEnvironmentVariable("FLIPMAN_APP_SCREENSHOT")), "screenshot failed");
            }
            auto* render = viewer.findChild<RenderView*>();
            require(render && render->viewport()->height() > render->height() * .7, "zoom bar stole viewport height");
            if (external.isEmpty()) {
                viewer.findChild<QPushButton*>("endButton")->click();
                require(viewer.session()->timeline()->time().frames() == 23, "transport control did not seek");
                viewer.session()->commands()->undo();
                require(viewer.session()->timeline()->time().frames() == 0, "transport undo failed");
                auto* fps = viewer.findChild<QComboBox*>("fps");
                fps->setCurrentIndex(fps->findData(30.0));
                require(viewer.session()->timeline()->fps() == av::Fps::fps30(), "FPS control ignored selection");
                viewer.session()->commands()->undo();
                QPointer<av::Timeline> oldTimeline = viewer.session()->timeline();
                QMimeData mime;
                mime.setUrls({ QUrl::fromLocalFile(path) });
                QDragEnterEvent enter(QPoint(20, 20), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(render->viewport(), &enter);
                require(enter.isAccepted(), "viewport rejected movie drag");
                QDropEvent drop(QPointF(20, 20), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(render->viewport(), &drop);
                waitFor([&]() { return !viewer.session()->isLoading(); });
                require(drop.isAccepted() && !oldTimeline && !viewer.session()->commands()->canUndo(),
                        "drop did not create a fresh session timeline");
            }
            viewer.showPython();
            QApplication::processEvents();
            if (external.isEmpty()) {
                viewer.findChild<QPushButton*>("runButton")->click();
                require(viewer.findChild<QPlainTextEdit*>("output")->toPlainText().contains(path),
                        "console Run did not use live session");
            }
        }
        else {
            auto& session = *application.session();
            if (mode == "view_lifetime")
                viewLifetime(application, path);
            else if (mode == "services")
                services(application);
            else if (mode == "mcp")
                mcp(application, path);
            else if (mode == "session")
                model(&session, path, directory.path());
            else if (mode == "commands")
                commands(&session, path);
            else if (mode == "python")
                python(&session, path);
            else
                require(false, "unknown case");
        }
        qInfo("Application regression passed");
        return 0;
    } catch (const std::exception& error) {
        qCritical("%s", error.what());
        return 1;
    }
}
