// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/rendercontext.h>

namespace flipman::sdk::render {

class RenderContextPrivate : public QSharedData {
public:
    struct Data {
        QRhi* rhi = nullptr;
        RenderSurface surface;
    };

    Data d;
};

RenderContext::RenderContext()
    : p(new RenderContextPrivate())
{}

RenderContext::RenderContext(const RenderContext& other)
    : p(other.p)
{}

RenderContext::~RenderContext() {}

QRhi*
RenderContext::rhi() const
{
    return p->d.rhi;
}

void
RenderContext::setRhi(QRhi* rhi)
{
    p->d.rhi = rhi;
}

RenderSurface
RenderContext::surface() const
{
    return p->d.surface;
}

void
RenderContext::setSurface(const RenderSurface& surface)
{
    p->d.surface = surface;
}

bool
RenderContext::isValid() const
{
    return p->d.rhi != nullptr;
}

void
RenderContext::reset()
{
    p.reset(new RenderContextPrivate());
}

RenderContext&
RenderContext::operator=(const RenderContext& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
RenderContext::operator==(const RenderContext& other) const
{
    return p->d.rhi == other.p->d.rhi && p->d.surface == other.p->d.surface;
}

bool
RenderContext::operator!=(const RenderContext& other) const
{
    return !(*this == other);
}

}  // namespace flipman::sdk::render
