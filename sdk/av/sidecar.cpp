// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <av/sidecar.h>

namespace av {
class SidecarPrivate : public QSharedData {};

Sidecar::Sidecar()
    : p(new SidecarPrivate())
{}

Sidecar::Sidecar(const Sidecar& other)
    : p(other.p)
{}

Sidecar::~Sidecar() {}

bool
Sidecar::is_valid() const
{
    return true;
}

void
Sidecar::reset()
{}

Sidecar&
Sidecar::operator=(const Sidecar& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}
}  // namespace av
