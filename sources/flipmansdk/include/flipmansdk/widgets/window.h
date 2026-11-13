// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <QMainWindow>
#include <QScopedPointer>

namespace flipman::sdk::widgets {

class WindowPrivate;

/**
 * @class Window
 * @brief Main application window for Flipman.
 *
 * Provides the top-level user interface and hosts the application's
 * menus, tool panels, tabbed views, status bar, and workspace widgets.
 *
 * The window uses a private implementation to manage UI construction,
 * styling, layout, and application-specific interactions.
 */
class FLIPMANSDK_EXPORT Window : public QMainWindow {
public:
    /**
     * @brief Constructs the main application window.
     *
     * Creates and initializes the user interface, including menus,
     * tool panels, central views, and status widgets.
     *
     * @param parent Optional parent widget.
     */
    explicit Window(QWidget* parent = nullptr);

    /**
     * @brief Destroys the window and releases associated resources.
     */
    ~Window() override;

private:
    Q_DISABLE_COPY_MOVE(Window)

    friend class WindowPrivate;

    /// Private implementation.
    QScopedPointer<WindowPrivate> p;
};

}  // namespace flipman::sdk::widgets

/**
 * @brief Registers Window pointers for QVariant and queued connections.
 */
Q_DECLARE_METATYPE(flipman::sdk::widgets::Window*)
