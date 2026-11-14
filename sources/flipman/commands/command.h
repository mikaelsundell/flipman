// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QString>
#include <functional>
#include <utility>

namespace flipman {

class Session;

/**
 * @brief
 * Session operation with an inverse, executed through CommandStack.
 */
class Command {
public:
    /**
     * @brief
     * Session operation returning whether the requested state change succeeded.
     */
    using Func = std::function<bool(Session*)>;

    /**
     * @brief
     * Stores a named operation and its inverse; the command stack takes ownership on submission.
     */
    Command(QString text, Func redo, Func undo)
        : d { std::move(text), std::move(redo), std::move(undo) }
    {}

    /**
     * @brief
     * Returns the human-readable description of the operation.
     */
    QString text() const { return d.text; }

    /**
     * @brief
     * Applies the stored forward operation and returns whether it succeeded.
     */
    bool execute(Session* session) { return d.redo && d.redo(session); }

    /**
     * @brief
     * Applies the previous command inverse when available.
     */
    bool undo(Session* session) { return d.undo && d.undo(session); }

private:
    struct Data {
        QString text;
        Func redo;
        Func undo;
    };
    Data d;
};

}  // namespace flipman
