// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QTextStream>

namespace flipman::sdk::log {
/**
 * @brief Provides a stream for standard output.
 * * This stream directs messages to the standard output (stdout),
 * typically used for general information, status updates, and
 * non-critical tracing.
 * * @return A reference to the global QTextStream for output.
 */
FLIPMANSDK_EXPORT QTextStream&
out();

/**
 * @brief Provides a stream for standard error.
 * * This stream directs messages to the standard error (stderr),
 * ensuring that error logs and critical warnings are separated
 * from standard program output.
 * * @return A reference to the global QTextStream for errors.
 */
FLIPMANSDK_EXPORT QTextStream&
err();

}  // namespace flipman::sdk::log
