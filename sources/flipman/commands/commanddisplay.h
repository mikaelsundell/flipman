// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include "command.h"

#include <flipmansdk/render/colorpipeline.h>

namespace flipman {

/**
 * @brief
 * Creates an undoable surface-tag edit; ownership transfers to CommandStack on submission.
 */
Command*
displayCommand(const sdk::render::DisplayTransform& before, const sdk::render::DisplayTransform& after);

/**
 * @brief
 * Creates an undoable color-pipeline edit and restores the prior manual surface tag on undo.
 */
Command*
colorCommand(const sdk::render::ColorSettings& before, const QString& beforeInput,
             const sdk::render::ColorSettings& after, const QString& afterInput,
             const sdk::render::DisplayTransform& beforeTag);

}  // namespace flipman
