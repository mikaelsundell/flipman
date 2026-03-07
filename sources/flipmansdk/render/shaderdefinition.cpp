// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell

#include <flipmansdk/render/shaderdefinition.h>

namespace flipman::sdk::render {

class ShaderDefinitionPrivate : public QSharedData {
public:
    ShaderDefinitionPrivate();
    ~ShaderDefinitionPrivate();

public:
    struct Data {
        QString sourceCode;
        QString shaderCode;
        QString uniformBlock;
        QString applyCode;
        QStringList includes;
        ShaderDefinition::ShaderDescriptor descriptor;
        core::Error error;
    };
    Data d;
};

ShaderDefinitionPrivate::ShaderDefinitionPrivate() {}

ShaderDefinitionPrivate::~ShaderDefinitionPrivate() {}

ShaderDefinition::ShaderDefinition()
    : p(new ShaderDefinitionPrivate)
{}

ShaderDefinition::ShaderDefinition(const ShaderDefinition& other)
    : p(other.p)
{}

ShaderDefinition::~ShaderDefinition() = default;

ShaderDefinition&
ShaderDefinition::operator=(const ShaderDefinition& other)
{
    if (this != &other)
        p = other.p;
    return *this;
}

QString
ShaderDefinition::uniformBlock() const
{
    return p->d.uniformBlock;
}

void
ShaderDefinition::setUniformBlock(const QString& code)
{
    p.detach();
    p->d.uniformBlock = code;
}

QString
ShaderDefinition::shaderCode() const
{
    return p->d.shaderCode;
}

void
ShaderDefinition::setShaderCode(const QString& shaderCode)
{
    p.detach();
    p->d.shaderCode = shaderCode;
}

QString
ShaderDefinition::applyCode() const
{
    return p->d.applyCode;
}

void
ShaderDefinition::setApplyCode(const QString& code)
{
    p.detach();
    p->d.applyCode = code;
}

const ShaderDefinition::ShaderDescriptor&
ShaderDefinition::descriptor() const
{
    return p->d.descriptor;
}

void
ShaderDefinition::setDescriptor(const ShaderDescriptor& descriptor)
{
    p.detach();
    p->d.descriptor = descriptor;
}

core::Error
ShaderDefinition::error() const
{
    return p->d.error;
}

bool
ShaderDefinition::isValid() const
{
    return p->d.error.isValid();
}

int
ShaderDefinition::ShaderDescriptor::indexOf(const QString& name) const
{
    for (int i = 0; i < parameters.size(); ++i) {
        if (parameters[i].name == name)
            return i;
    }
    return -1;
}

}  // namespace flipman::sdk::render
