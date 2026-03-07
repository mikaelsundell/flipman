// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/imageeffect.h>

#include <QRegularExpression>
#include <QStringList>

namespace flipman::sdk::render {

class ImageEffectPrivate : public QSharedData {
public:
    ImageEffectPrivate();
    ~ImageEffectPrivate();
    struct Data {
        QString name;
        QString group;
        ShaderDefinition shaderDefinition;
        core::Error error;
    };
    Data d;
};

ImageEffectPrivate::ImageEffectPrivate() {}

ImageEffectPrivate::~ImageEffectPrivate() {}

ImageEffect::ImageEffect()
    : p(new ImageEffectPrivate())
{}

ImageEffect::ImageEffect(const ImageEffect& other)
    : p(other.p)
{}

ImageEffect::~ImageEffect() = default;

QString
ImageEffect::name() const
{
    return p->d.name;
}

void
ImageEffect::setName(const QString& name)
{
    if (p->d.name != name) {
        p->d.name = name;
    }
}

QString
ImageEffect::group() const
{
    return p->d.group;
}

void
ImageEffect::setGroup(const QString& group)
{
    if (p->d.group != group) {
        p->d.group = group;
    }
}

ShaderDefinition
ImageEffect::shaderDefinition() const
{
    return p->d.shaderDefinition;
}

void
ImageEffect::setShaderDefinition(const ShaderDefinition& shaderDefinition)
{
    p->d.shaderDefinition = shaderDefinition;
}

core::Error
ImageEffect::error() const
{
    return p->d.error;
}

void
ImageEffect::reset()
{
    p.reset(new ImageEffectPrivate());
}

bool
ImageEffect::isValid() const
{
    return p->d.shaderDefinition.isValid();
}

ImageEffect&
ImageEffect::operator=(const ImageEffect& other)
{
    if (this != &other)
        p = other.p;
    return *this;
}

bool
ImageEffect::operator==(const ImageEffect& other) const
{
    return p == other.p;
}

bool
ImageEffect::operator!=(const ImageEffect& other) const
{
    return !(*this == other);
}

}  // namespace flipman::sdk::render
