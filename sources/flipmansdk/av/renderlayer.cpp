// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <av/renderlayer.h>

#include <core/imagebuffer.h>

namespace flipman::sdk::av {
class RenderLayerPrivate : public QSharedData {
public:
    void init();
    struct Data {
        core::ImageBuffer image;
        RenderEffect renderEffect;
        QMatrix4x4 transform;
        core::Error error;
    };
    Data d;
};

void
RenderLayerPrivate::init()
{
    d.transform.setToIdentity();
}

RenderLayer::RenderLayer()
    : p(new RenderLayerPrivate())
{
    p->init();
}

RenderLayer::RenderLayer(const RenderLayer& other)
    : p(other.p)
{}

RenderLayer::~RenderLayer() {}

core::ImageBuffer
RenderLayer::image() const
{
    return p->d.image;
}

RenderEffect
RenderLayer::renderEffect() const
{
    return p->d.renderEffect;
}

QMatrix4x4
RenderLayer::transform() const
{
    return p->d.transform;
}

core::Error
RenderLayer::error() const
{
    return p->d.error;
}

void
RenderLayer::reset()
{
    p.reset(new RenderLayerPrivate());
}

void
RenderLayer::setImage(const core::ImageBuffer& image)
{
    p->d.image = image;
}

void
RenderLayer::setRenderEffect(const RenderEffect& renderEffect)
{
    p->d.renderEffect = renderEffect;
}

void
RenderLayer::setTransform(const QMatrix4x4& matrix)
{
    p->d.transform = matrix;
}

RenderLayer&
RenderLayer::operator=(const RenderLayer& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
RenderLayer::operator==(const RenderLayer& other) const
{
    return (p->d.image == other.p->d.image && p->d.renderEffect == other.p->d.renderEffect
            && p->d.transform == other.p->d.transform);
}

bool
RenderLayer::operator!=(const RenderLayer& other) const
{
    return !(*this == other);
}

}  // namespace flipman::sdk::av
