// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QObject>

namespace core {
class Object : public QObject {
public:
    Object(QObject* parent = nullptr);
    virtual ~Object();
    virtual bool is_valid() const = 0;
    virtual void reset() = 0;
};
}  // namespace core
