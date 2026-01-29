// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/plugin.h>

namespace flipman::sdk::core {
Plugin::Plugin(QObject* parent)
    : QObject(parent)
{}

Plugin::~Plugin() {}
}  // namespace flipman::sdk::core
