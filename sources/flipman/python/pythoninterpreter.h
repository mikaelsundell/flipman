// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QObject>
#include <QScopedPointer>

namespace flipman {

class Session;
class PythonInterpreterPrivate;

/**
 * @brief
 * Persistent in-process Python console. Borrows SDK services and the session.
 *
 * Created and used on the GUI thread; lazy initialization does not affect
 * playback. Scripts run synchronously and should keep long work off this thread.
 */
class PythonInterpreter : public QObject {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates an interpreter facade borrowing its session and SDK services; CPython starts lazily.
     */
    explicit PythonInterpreter(Session* session);

    /**
     * @brief
     * Finalizes owned CPython on the GUI thread before borrowed SDK and application services are destroyed.
     */
    ~PythonInterpreter() override;

    /**
     * @brief
     * Runs source synchronously on the GUI thread and returns captured stdout, stderr and tracebacks.
     */
    QString executeScript(const QString& script);

private:
    QScopedPointer<PythonInterpreterPrivate> p;
};

}  // namespace flipman
