// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/core/application.h>
#include <QEvent>
#include <QScopedPointer>

namespace flipman {

class ApplicationPrivate;
class Console;
class McpServer;
class PythonInterpreter;
class Session;
class Settings;

/**
 * @brief
 * SDK theme and resource facade reused by the application without another instance.
 */
using UI = sdk::core::UI;

/**
 * @brief
 * Main application and owner of shared application subsystems.
 *
 * Extends the SDK application to retain its UI, plugin registry and native
 * services. Construct on the GUI thread before any views, and destroy views
 * before this object. The interpreter is finalized before SDK services.
 */
class Application : public sdk::core::Application {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates subsystems and applies clean-run settings before creating views.
     *
     * Honors FLIPMAN_TEST_SETTINGS_DIR, --settings-dir and --clean-run.
     */
    Application(int& argc, char** argv);

    /**
     * @brief
     * Stops MCP, finalizes Python while services are live, and retires the decoder before SDK teardown.
     */
    ~Application() override;

    /**
     * @brief
     * Returns the application-owned native stdout/stderr capture backend.
     */
    Console* console() const;

    /**
     * @brief
     * Returns the application-owned interpreter; CPython initializes on first use.
     */
    PythonInterpreter* pythonInterpreter() const;

    /**
     * @brief
     * Returns the optional, initially stopped loopback MCP server.
     */
    McpServer* mcpServer() const;

    /**
     * @brief
     * Returns the application-owned document and playback controller.
     */
    Session* session() const;

    /**
     * @brief
     * Returns persistent or isolated preferences selected for this process.
     */
    Settings* settings() const;

    /**
     * @brief
     * Returns the existing SDK-owned UI facade, including display profiles.
     */
    UI* ui() const;

    /**
     * @brief
     * Returns the current main window, or null before a viewer is created.
     */
    QWidget* window() const;

    /**
     * @brief
     * Returns the process settings directory, or empty for normal preferences.
     */
    QString settingsDirectory() const;

    /**
     * @brief
     * Returns the global instance, terminating if an incompatible app is active.
     */
    static Application* instance();

Q_SIGNALS:
    /**
     * @brief
     * Forwards a movie/image open request received from the operating system.
     */
    void fileOpenRequested(const QString& filename);

protected:
    /**
     * @brief
     * Handles native file-open events and delegates other events to the SDK app.
     */
    bool event(QEvent* event) override;

private:
    Q_DISABLE_COPY_MOVE(Application)
    QScopedPointer<ApplicationPrivate> p;
};

/**
 * @brief
 * Returns the global Flipman application; use on the GUI thread.
 */
inline Application*
app()
{
    return Application::instance();
}

/**
 * @brief
 * Returns the shared application session without transferring ownership.
 */
inline Session*
session()
{
    return app()->session();
}

/**
 * @brief
 * Returns the shared Python interpreter without transferring ownership.
 */
inline PythonInterpreter*
pythonInterpreter()
{
    return app()->pythonInterpreter();
}

/**
 * @brief
 * Returns the optional MCP endpoint without starting it.
 */
inline McpServer*
mcpServer()
{
    return app()->mcpServer();
}

/**
 * @brief
 * Returns the shared native console backend without starting capture.
 */
inline Console*
console()
{
    return app()->console();
}

/**
 * @brief
 * Returns this process's application preferences.
 */
inline Settings*
settings()
{
    return app()->settings();
}

/**
 * @brief
 * Returns the existing SDK UI facade used by all application views.
 */
inline sdk::core::UI*
ui()
{
    return app()->ui();
}

}  // namespace flipman
