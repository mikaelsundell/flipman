// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QApplication>
#include <QScopedPointer>

namespace flipman::sdk::core {

class Environment;
class Style;
class System;
class ApplicationPrivate;

/**
 * @class Application
 * @brief The primary application class for the Flipman SDK.
 * * Extends QApplication to provide centralized access to core SDK subsystems
 * including Environment, Style, and System management.
 */
class FLIPMANSDK_EXPORT Application : public QApplication {
    Q_OBJECT
public:
    /**
     * @brief Initializes the SDK application.
     * @param argc Argument count from main.
     * @param argv Argument vector from main.
     */
    Application(int& argc, char** argv);

    /**
     * @brief Destroys the application.
     * @note Overrides the virtual destructor to ensure safe SDK teardown.
     */
    ~Application() override;

    /**
     * @brief Returns the environment manager for path and resource resolution.
     */
    Environment* environment() const;

    /**
     * @brief Returns the global style manager for themes and UI branding.
     */
    Style* style() const;

    /**
     * @brief Returns the system manager for power and hardware interaction.
     */
    System* system() const;

    /**
     * @brief Returns the global Application instance.
     * @note Will assert if the current QCoreApplication is not a core::Application.
     */
    static Application* instance();

private:
    Q_DISABLE_COPY_MOVE(Application)
    QScopedPointer<ApplicationPrivate> p;
};

/**
 * @brief Convenience helper to access the global Application instance.
 */
inline Application*
app()
{
    return Application::instance();
}

/**
 * @brief Global accessor for the SDK Environment subsystem.
 */
inline Environment*
environment()
{
    auto* a = app();
    return a ? a->environment() : nullptr;
}

/**
 * @brief Global accessor for the SDK Style subsystem.
 */
inline Style*
style()
{
    auto* a = app();
    return a ? a->style() : nullptr;
}

/**
 * @brief Global accessor for the SDK System subsystem.
 */
inline System*
system()
{
    auto* a = app();
    return a ? a->system() : nullptr;
}

}  // namespace flipman::sdk::core
