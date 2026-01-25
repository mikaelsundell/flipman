// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/object.h>

#include <QExplicitlySharedDataPointer>

namespace av {
class SidecarPrivate;
class Sidecar : public core::Object {
public:
    Sidecar();
    Sidecar(const Sidecar& other);
    virtual ~Sidecar();
    bool is_valid() const override;
    void reset() override;

    Sidecar& operator=(const Sidecar& other);

private:
    QExplicitlySharedDataPointer<SidecarPrivate> p;
};
}  // namespace av
