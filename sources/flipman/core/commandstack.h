// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QObject>
#include <QScopedPointer>

namespace flipman {

class Session;
class Command;
class CommandStackPrivate;

/**
 * @brief
 * Session-owned, bounded undo/redo history; playback ticks never enter history.
 */
class CommandStack : public QObject {
    Q_OBJECT
public:
    /**
     * @brief
     * Distinguishes a new operation from an edit already previewed during a scrub.
     */
    enum class ExecutionMode {
        /**
         * @brief
         * Executes the command before adding it to undo history.
         */
        Execute,
        /**
         * @brief
         * Records an existing edit without executing it a second time.
         */
        Applied
    };

    /**
     * @brief
     * Creates bounded history for the borrowed session.
     */
    explicit CommandStack(Session* session);

    /**
     * @brief
     * Releases all owned commands without applying further edits.
     */
    ~CommandStack() override;

    /**
     * @brief
     * Takes ownership of a command and executes it, or records an already-applied edit.
     */
    void execute(Command* command, ExecutionMode mode = ExecutionMode::Execute);

    /**
     * @brief
     * Returns whether a session command can be undone.
     */
    bool canUndo() const;

    /**
     * @brief
     * Returns whether an undone session command can be reapplied.
     */
    bool canRedo() const;

public Q_SLOTS:
    /**
     * @brief
     * Applies the previous command inverse when available.
     */
    void undo();

    /**
     * @brief
     * Reapplies the next undone command when available.
     */
    void redo();

    /**
     * @brief
     * Drops all history when a document is closed or replaced.
     */
    void clear();

Q_SIGNALS:
    /**
     * @brief
     * Reports an undo/redo index change or cleared command history.
     */
    void changed();

private:
    QScopedPointer<CommandStackPrivate> p;
};

}  // namespace flipman
