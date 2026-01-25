// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <core/container.h>

namespace core {
Container::Container(QObject* parent)
    : QObject(parent)
{}

Container::~Container() {}
}  // namespace core
