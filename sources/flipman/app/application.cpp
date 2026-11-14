// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "application.h"
#include "core/console.h"
#include "core/session.h"
#include "core/settings.h"
#include "mcp/mcpserver.h"
#include "python/pythoninterpreter.h"

#include <QDir>
#include <QFileOpenEvent>
#include <QMainWindow>
#include <QSettings>
#include <QTemporaryDir>

namespace flipman {

class ApplicationPrivate {
public:
    struct Data {
        QScopedPointer<Console> console;
        QScopedPointer<Session> session;
        QScopedPointer<PythonInterpreter> interpreter;
        QScopedPointer<Settings> settings;
        QScopedPointer<McpServer> mcp;
        QScopedPointer<QTemporaryDir> temporarySettings;
        QString settingsDirectory;
    };
    Data d;
};

Application::Application(int& argc, char** argv)
    : sdk::core::Application(argc, argv)
    , p(new ApplicationPrivate)
{
    setApplicationName("Flipman");
    setOrganizationName("Mikael Sundell");
    setOrganizationDomain("mikaelsundell.com");
    QString root = qEnvironmentVariable("FLIPMAN_TEST_SETTINGS_DIR");
    const auto arguments = QCoreApplication::arguments();
    for (int i = 1; i < arguments.size(); ++i) {
        if (arguments[i].startsWith("--settings-dir="))
            root = arguments[i].mid(15);
        else if (arguments[i] == "--settings-dir" && i + 1 < arguments.size())
            root = arguments[++i];
    }
    if (root.isEmpty() && arguments.contains("--clean-run")) {
        p->d.temporarySettings.reset(new QTemporaryDir(QDir::tempPath() + "/flipman-clean-XXXXXX"));
        if (!p->d.temporarySettings->isValid())
            qFatal("Could not create clean-run settings directory");
        root = p->d.temporarySettings->path();
    }
    if (!root.isEmpty()) {
        p->d.settingsDirectory = QDir(root).absoluteFilePath(QString::number(applicationPid()));
        if (!QDir().mkpath(p->d.settingsDirectory))
            qFatal("Could not create isolated Flipman settings directory");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, p->d.settingsDirectory);
    }
    p->d.settings.reset(new Settings(p->d.settingsDirectory));
    p->d.console.reset(new Console);
    p->d.session.reset(new Session);
    p->d.interpreter.reset(new PythonInterpreter(p->d.session.data()));
    p->d.mcp.reset(new McpServer);
}

Application::~Application()
{
    p->d.mcp->stop();
    p->d.interpreter.reset();
    p->d.mcp.reset();
    p->d.session.reset();
    p->d.console->stop();
    p->d.console.reset();
    p->d.settings.reset();
}

Console*
Application::console() const
{
    return p->d.console.data();
}

PythonInterpreter*
Application::pythonInterpreter() const
{
    return p->d.interpreter.data();
}

McpServer*
Application::mcpServer() const
{
    return p->d.mcp.data();
}

Session*
Application::session() const
{
    return p->d.session.data();
}

Settings*
Application::settings() const
{
    return p->d.settings.data();
}

UI*
Application::ui() const
{
    return sdk::core::Application::ui();
}

QString
Application::settingsDirectory() const
{
    return p->d.settingsDirectory;
}

QWidget*
Application::window() const
{
    for (auto* widget : topLevelWidgets())
        if (qobject_cast<QMainWindow*>(widget))
            return widget;

    return nullptr;
}

Application*
Application::instance()
{
    auto* result = qobject_cast<Application*>(QCoreApplication::instance());
    if (!result)
        qFatal("Create flipman::Application before using application subsystems");
    return result;
}

bool
Application::event(QEvent* event)
{
    if (event && event->type() == QEvent::FileOpen) {
        const auto filename = static_cast<QFileOpenEvent*>(event)->file();
        if (!filename.isEmpty()) {
            Q_EMIT fileOpenRequested(filename);
            return true;
        }
    }
    return sdk::core::Application::event(event);
}

}  // namespace flipman
