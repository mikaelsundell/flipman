// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "app/viewer.h"
#include "core/console.h"
#include "core/session.h"
#include "core/settings.h"
#include "mcp/mcpserver.h"
#include "python/pythoninterpreter.h"

#include <QCommandLineParser>
#include <QDebug>
#include <QTimer>

int
main(int argc, char** argv)
{
    flipman::Application application(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Flipman movie and image player");
    parser.addHelpOption();
    parser.addOption({ "open", "Open a movie or image.", "file" });
    parser.addOption({ "execute", "Execute Python after startup (after the requested file opens).", "code" });
    parser.addOption({ "clean-run", "Use temporary, isolated preferences instead of normal user settings." });
    parser.addOption({ "settings-dir", "Use isolated preferences under DIR/PID.", "directory" });
    parser.addOption({ "mcp", "Start the optional loopback MCP endpoint." });
    parser.addOption({ "mcp-port", "MCP TCP port; 0 requests an available port.", "port" });
    parser.addOption({ "console", "Capture native stdout/stderr in the application console backend." });
    parser.addPositionalArgument("file", "Movie or image to open.", "[file]");
    parser.process(application);
    if (parser.positionalArguments().size() > 1)
        parser.showHelp(2);
    if (parser.isSet("mcp-port")) {
        bool valid = false;
        const auto port = parser.value("mcp-port").toInt(&valid);
        if (!valid || port < 0 || port > 65535) {
            qCritical("MCP port must be 0..65535");
            return 2;
        }
        qputenv("FLIPMAN_TEST_MCP_PORT", QByteArray::number(port));
    }
    flipman::Viewer viewer;
    QObject::connect(&application, &flipman::Application::fileOpenRequested, &viewer, &flipman::Viewer::openFile);
    viewer.show();
    const auto file = parser.isSet("open") ? parser.value("open") : parser.positionalArguments().value(0);
    const auto code = parser.value("execute");
    QTimer::singleShot(0, &viewer,
                       [&application, file, code, capture = parser.isSet("console"), serve = parser.isSet("mcp")]() {
                           if (capture)
                               application.console()->start();
                           if (serve || application.settings()->value("mcp/enabled", false).toBool()) {
                               if (!application.mcpServer()->start())
                                   qWarning().noquote() << application.mcpServer()->errorString();
                               else
                                   qInfo().noquote() << "MCP:" << application.mcpServer()->url().toString();
                           }
                           const auto execute = [&application, code]() {
                               if (!code.isEmpty())
                                   qInfo().noquote() << application.pythonInterpreter()->executeScript(code);
                           };
                           if (!file.isEmpty()) {
                               if (!code.isEmpty())
                                   QObject::connect(application.session(), &flipman::Session::opened, &application,
                                                    execute, Qt::SingleShotConnection);
                               application.session()->openFile(file);
                           }
                           else
                               execute();
                       });
    return application.exec();
}
