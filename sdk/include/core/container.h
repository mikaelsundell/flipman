// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/error.h>

#include <QObject>

namespace core {
class Container : public QObject {
public:
    Container(QObject* parent = nullptr);
    virtual ~Container();
    virtual core::Error error() const = 0;
    virtual void reset() = 0;
};
}  // namespace core
