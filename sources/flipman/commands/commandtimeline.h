// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include "command.h"

#include <flipmansdk/av/time.h>

namespace flipman {

/**
 * @brief
 * Creates an undoable timeline seek; decoded presentation completes asynchronously.
 */
Command*
timeCommand(const sdk::av::Time& before, const sdk::av::Time& after);

/**
 * @brief
 * Creates an undoable FPS change that preserves timeline frame indices.
 */
Command*
fpsCommand(const sdk::av::Fps& before, const sdk::av::Fps& after);

}  // namespace flipman
