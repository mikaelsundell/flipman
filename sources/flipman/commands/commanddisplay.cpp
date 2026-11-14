// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "commanddisplay.h"
#include "core/session.h"

namespace flipman {

Command*
displayCommand(const sdk::render::DisplayTransform& before, const sdk::render::DisplayTransform& after)
{
    return new Command(
        "Set display profile", [after](Session* s) { return s->setDisplayTransform(after); },
        [before](Session* s) { return s->setDisplayTransform(before); });
}

Command*
colorCommand(const sdk::render::ColorSettings& before, const QString& beforeInput,
             const sdk::render::ColorSettings& after, const QString& afterInput,
             const sdk::render::DisplayTransform& beforeTag)
{
    return new Command(
        "Set color transform",
        [after, afterInput](Session* s) {
            if (!s->setColorSettings(after, afterInput))
                return false;

            sdk::render::DisplayTransform tag;
            if (sdk::render::ColorPipeline::displayTag(after.display, tag))
                s->setDisplayTransform(tag);
            return true;
        },
        [before, beforeInput, beforeTag](Session* s) {
            if (!s->setColorSettings(before, beforeInput))
                return false;

            return s->setDisplayTransform(beforeTag);
        });
}

}  // namespace flipman
