// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/render/imagelayer.h>

namespace flipman::sdk::render {
class ImageLayerPrivate : public QSharedData {
public:
    void init();
    struct Data {
        core::ImageBuffer image;
        ImageEffect imageEffect;
        QMatrix4x4 transform;
        core::Error error;
    };
    Data d;
};

void
ImageLayerPrivate::init()
{
    d.transform.setToIdentity();
}

ImageLayer::ImageLayer()
    : p(new ImageLayerPrivate())
{
    p->init();
}

ImageLayer::ImageLayer(const ImageLayer& other)
    : p(other.p)
{}

ImageLayer::~ImageLayer() {}

core::ImageBuffer
ImageLayer::image() const
{
    return p->d.image;
}

ImageEffect
ImageLayer::imageEffect() const
{
    return p->d.imageEffect;
}

QMatrix4x4
ImageLayer::transform() const
{
    return p->d.transform;
}

core::Error
ImageLayer::error() const
{
    return p->d.error;
}

void
ImageLayer::reset()
{
    p.reset(new ImageLayerPrivate());
}

void
ImageLayer::setImage(const core::ImageBuffer& image)
{
    p->d.image = image;
}

void
ImageLayer::setImageEffect(const ImageEffect& imageEffect)
{
    p->d.imageEffect = imageEffect;
}

void
ImageLayer::setTransform(const QMatrix4x4& matrix)
{
    p->d.transform = matrix;
}

ImageLayer&
ImageLayer::operator=(const ImageLayer& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
ImageLayer::operator==(const ImageLayer& other) const
{
    return (p->d.image == other.p->d.image && p->d.imageEffect == other.p->d.imageEffect
            && p->d.transform == other.p->d.transform);
}

bool
ImageLayer::operator!=(const ImageLayer& other) const
{
    return !(*this == other);
}

}  // namespace flipman::sdk::render
