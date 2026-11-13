// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <QLoggingCategory>
#include <QTextStream>

namespace flipman::sdk::core {

/**
 * @brief Media diagnostics; debug messages are disabled by default.
 *
 * Enable with QT_LOGGING_RULES="flipman.media.debug=true".
 */
FLIPMANSDK_EXPORT const QLoggingCategory&
mediaLog();

/**
 * @brief Rendering diagnostics; debug messages are disabled by default.
 *
 * Enable with QT_LOGGING_RULES="flipman.render.debug=true".
 */
FLIPMANSDK_EXPORT const QLoggingCategory&
renderLog();

/**
 * @brief Returns the standard output stream.
 */
FLIPMANSDK_EXPORT QTextStream&
logOut();

/**
 * @brief Returns the standard error stream.
 */
FLIPMANSDK_EXPORT QTextStream&
logErr();

}  // namespace flipman::sdk::core
