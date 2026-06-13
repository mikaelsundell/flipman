// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/rendersurface.h>

namespace flipman::sdk::render {

class RenderSurfacePrivate : public QSharedData {
public:
    struct Data {
        QRhiRenderTarget* renderTarget = nullptr;
    };

    Data d;
};

RenderSurface::RenderSurface()
    : p(new RenderSurfacePrivate())
{}

RenderSurface::RenderSurface(const RenderSurface& other)
    : p(other.p)
{}

RenderSurface::~RenderSurface() {}

QRhiRenderTarget*
RenderSurface::renderTarget() const
{
    return p->d.renderTarget;
}

void
RenderSurface::setRenderTarget(QRhiRenderTarget* renderTarget)
{
    p->d.renderTarget = renderTarget;
}

bool
RenderSurface::isValid() const
{
    return p->d.renderTarget != nullptr;
}

void
RenderSurface::reset()
{
    p.reset(new RenderSurfacePrivate());
}

RenderSurface&
RenderSurface::operator=(const RenderSurface& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
RenderSurface::operator==(const RenderSurface& other) const
{
    return p->d.renderTarget == other.p->d.renderTarget;
}

bool
RenderSurface::operator!=(const RenderSurface& other) const
{
    return !(*this == other);
}

}  // namespace flipman::sdk::render
