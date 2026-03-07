// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QApplication>
#include <QScopedPointer>

namespace flipman::sdk::plugins {
class PluginRegistry;
}

namespace flipman::sdk::core {

class Environment;
class Style;
class System;
class ApplicationPrivate;

/**
 * @class Application
 * @brief Root application object for the Flipman SDK.
 *
 * Extends QApplication and provides access to core subsystems.
 */
class FLIPMANSDK_EXPORT Application : public QApplication {
    Q_OBJECT
public:
    /**
     * @brief Constructs the SDK application.
     *
     * @param argc Argument count.
     * @param argv Argument vector.
     */
    Application(int& argc, char** argv);

    /**
     * @brief Destroys the application.
     */
    ~Application() override;

    /**
     * @brief Returns the Environment subsystem.
     */
    Environment* environment() const;

    /**
     * @brief Returns the Style subsystem.
     */
    Style* style() const;

    /**
     * @brief Returns the System subsystem.
     */
    System* system() const;

    /**
     * @brief Returns the global PluginRegistry.
     */
    plugins::PluginRegistry* pluginRegistry() const;

    /**
     * @brief Returns the global Application instance.
     *
     * Asserts if the active QCoreApplication is not a core::Application.
     */
    static Application* instance();

private:
    Q_DISABLE_COPY_MOVE(Application)
    QScopedPointer<ApplicationPrivate> p;
};

/**
 * @brief Returns the global Application instance.
 */
inline Application*
app()
{
    return Application::instance();
}

/**
 * @brief Returns the global Environment subsystem.
 */
inline Environment*
environment()
{
    auto* a = app();
    return a ? a->environment() : nullptr;
}

/**
 * @brief Returns the global Style subsystem.
 */
inline Style*
style()
{
    auto* a = app();
    return a ? a->style() : nullptr;
}

/**
 * @brief Returns the global System subsystem.
 */
inline System*
system()
{
    auto* a = app();
    return a ? a->system() : nullptr;
}

/**
 * @brief Returns the global Plugin registery subsystem.
 */
inline plugins::PluginRegistry*
pluginRegistry()
{
    auto* a = app();
    return a ? a->pluginRegistry() : nullptr;
}

}  // namespace flipman::sdk::core
