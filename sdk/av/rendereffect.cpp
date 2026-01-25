// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <av/rendereffect.h>

namespace av {
class RenderEffectPrivate : public QSharedData {
public:
    struct Data {
        core::Parameters parameters;
        QString code;
        core::Error error;
    };
    Data d;
};

RenderEffect::RenderEffect()
    : p(new RenderEffectPrivate())
{}

RenderEffect::RenderEffect(const RenderEffect& other)
    : p(other.p)
{}

RenderEffect::~RenderEffect() {}

core::Parameters
RenderEffect::parameters() const
{
    return p->d.parameters;
}

QString
RenderEffect::code() const
{
    return p->d.code;
}

core::Error
RenderEffect::error() const
{
    return p->d.error;
}

void
RenderEffect::reset()
{
    p.reset(new RenderEffectPrivate());
}

void
RenderEffect::set_parameters(const core::Parameters& parameters)
{
    p->d.parameters = parameters;
}

void
RenderEffect::set_code(const QString& code)
{
    p->d.code = code;
}

RenderEffect&
RenderEffect::operator=(const RenderEffect& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
RenderEffect::operator==(const RenderEffect& other) const
{
    return this->p == other.p;
}

bool
RenderEffect::operator!=(const RenderEffect& other) const
{
    return !(this->p == other.p);
}
}  // namespace av
