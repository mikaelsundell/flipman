// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QMainWindow>
#include <QScopedPointer>

namespace flipman {

class Session;
class ViewerPrivate;

/**
 * @brief
 * Main application shell. Views consume the session rather than own media.
 */
class Viewer : public QMainWindow {
    Q_OBJECT
public:
    /**
     * @brief
     * Builds the main window around application-owned subsystems and preferences.
     */
    explicit Viewer(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~Viewer() override;

    /**
     * @brief
     * Returns the borrowed application session; no document or decoder is owned by the view.
     */
    Session* session() const;

    /**
     * @brief
     * Opens an optional positional file after the viewer has been constructed.
     */
    void setArguments(const QStringList& arguments);

public Q_SLOTS:
    /**
     * @brief
     * Requests asynchronous file opening through the session; does not write source files.
     */
    void openFile(const QString& filename);

    /**
     * @brief
     * Shows or raises the editor for the existing application interpreter.
     */
    void showPython();

protected:
    /**
     * @brief
     * Accepts a single local file for opening through the session.
     */
    void dragEnterEvent(QDragEnterEvent* event) override;

    /**
     * @brief
     * Routes a dropped local file through the normal asynchronous open path.
     */
    void dropEvent(QDropEvent* event) override;

    /**
     * @brief
     * Stops playback and stores window geometry in this process's settings backend.
     */
    void closeEvent(QCloseEvent* event) override;

private:
    QScopedPointer<ViewerPrivate> p;
};

}  // namespace flipman
