// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <core/object.h>

namespace core {
Object::Object(QObject* parent)
    : QObject(parent)
{}

Object::~Object() {}
}  // namespace core
