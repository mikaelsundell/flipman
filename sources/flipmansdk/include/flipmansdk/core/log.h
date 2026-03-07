// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QTextStream>

namespace flipman::sdk::core {

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
