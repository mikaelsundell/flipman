// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <av/renderlayer.h>

namespace av {
class RenderLayerPrivate : public QSharedData {
public:
    void init();
    struct Data {
        core::ImageBuffer image;
        RenderEffect rendereffect;
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
RenderLayer::rendereffect() const
{
    return p->d.rendereffect;
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
RenderLayer::set_image(const core::ImageBuffer& image)
{
    p->d.image = image;
}

void
RenderLayer::set_rendereffect(const RenderEffect& rendereffect)
{
    p->d.rendereffect = rendereffect;
}

void
RenderLayer::set_transform(const QMatrix4x4& matrix)
{
    p->d.transform = matrix;
}
}  // namespace av
