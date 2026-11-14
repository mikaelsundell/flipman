// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "commandtimeline.h"
#include "core/session.h"

namespace flipman {

Command*
timeCommand(const sdk::av::Time& before, const sdk::av::Time& after)
{
    return new Command(
        "Set time", [after](Session* s) { return s->setFrame(after.frames()); },
        [before](Session* s) { return s->setFrame(before.frames()); });
}

Command*
fpsCommand(const sdk::av::Fps& before, const sdk::av::Fps& after)
{
    return new Command(
        "Set FPS", [after](Session* s) { return s->setFps(after); },
        [before](Session* s) { return s->setFps(before); });
}

}  // namespace flipman
