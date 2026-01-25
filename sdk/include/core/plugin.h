// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/error.h>

#include <QObject>

namespace core {
class Plugin : public QObject {
public:
    Plugin(QObject* parent = nullptr);
    virtual ~Plugin();
    virtual Error error() const = 0;
};
}  // namespace core
