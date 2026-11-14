// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "commands/command.h"
#include "commandstack.h"
#include "session.h"

#include <QPointer>
#include <QUndoCommand>
#include <QUndoStack>
#include <memory>

namespace flipman {

class SessionCommand : public QUndoCommand {
public:
    SessionCommand(Session* session, Command* command, bool applied)
        : d { std::unique_ptr<Command>(command), applied, session }
    {
        setText(command->text());
    }

    void redo() override
    {
        if (d.applied) {
            d.applied = false;
            return;
        }
        if (!d.session || !d.command->execute(d.session))
            setObsolete(true);
    }

    void undo() override
    {
        if (!d.session || !d.command->undo(d.session))
            setObsolete(true);
    }

private:
    struct Data {
        std::unique_ptr<Command> command;
        bool applied;
        QPointer<Session> session;
    };
    Data d;
};

class CommandStackPrivate {
public:
    struct Data {
        QUndoStack history;
        QPointer<Session> session;
    };
    Data d;
};

CommandStack::CommandStack(Session* session)
    : QObject(session)
    , p(new CommandStackPrivate)
{
    p->d.session = session;
    p->d.history.setUndoLimit(100);
    connect(&p->d.history, &QUndoStack::indexChanged, this, &CommandStack::changed);
}

CommandStack::~CommandStack() = default;

void
CommandStack::execute(Command* command, ExecutionMode mode)
{
    if (command)
        p->d.history.push(new SessionCommand(p->d.session, command, mode == ExecutionMode::Applied));
}

bool
CommandStack::canUndo() const
{
    return p->d.history.canUndo();
}

bool
CommandStack::canRedo() const
{
    return p->d.history.canRedo();
}

void
CommandStack::undo()
{
    p->d.history.undo();
}

void
CommandStack::redo()
{
    p->d.history.redo();
}

void
CommandStack::clear()
{
    p->d.history.clear();
}

}  // namespace flipman
