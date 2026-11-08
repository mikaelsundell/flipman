// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <QSharedData>
#include <flipmansdk/av/sidecar.h>

namespace flipman::sdk::av {
class SideCarPrivate : public QSharedData {};

SideCar::SideCar()
    : p(new SideCarPrivate())
{}

SideCar::SideCar(const SideCar& other)
    : p(other.p)
{}

SideCar::~SideCar() {}

bool
SideCar::isValid() const
{
    return true;
}

void
SideCar::reset()
{}

SideCar&
SideCar::operator=(const SideCar& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}
}  // namespace flipman::sdk::av
