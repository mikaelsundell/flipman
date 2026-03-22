// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell

#include <flipmansdk/render/shaderdefinition.h>

namespace flipman::sdk::render {

using ShaderParameterType = ShaderDescriptor::ShaderParameter::Type;

class ShaderDefinitionPrivate : public QSharedData {
public:
    ShaderDefinitionPrivate();
    ~ShaderDefinitionPrivate();
    QString parameterType(ShaderParameterType type) const;

public:
    struct Data {
        QString shaderCode;
        QStringList includes;
        ShaderDescriptor descriptor;
        QVector<ShaderFunction> functions;
        core::Error error;
    };
    Data d;
};

ShaderDefinitionPrivate::ShaderDefinitionPrivate() {}

ShaderDefinitionPrivate::~ShaderDefinitionPrivate() {}

QString
ShaderDefinitionPrivate::parameterType(ShaderParameterType type) const
{
    switch (type) {
    case ShaderParameterType::Float: return "float";
    case ShaderParameterType::Int: return "int";
    case ShaderParameterType::Bool: return "bool";
    case ShaderParameterType::Vec2: return "vec2";
    case ShaderParameterType::Vec3: return "vec3";
    case ShaderParameterType::Vec4: return "vec4";
    }
    return "float";
}

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

const ShaderDescriptor&
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

QVector<ShaderFunction>
ShaderDefinition::functions() const
{
    return p->d.functions;
}

void
ShaderDefinition::setFunctions(const QVector<ShaderFunction>& functions)
{
    p.detach();
    p->d.functions = functions;
}

QString
ShaderDefinition::uniformBlock(int binding, const QString& uniformName) const
{
    QString code;
    QTextStream stream(&code);
    if (!p->d.descriptor.parameters.isEmpty()) {
        stream << "layout(std140, binding = " << binding << ") uniform " << uniformName << "\n{\n";
        for (const auto& param : p->d.descriptor.parameters)
            stream << "    " << p->parameterType(param.type) << " " << param.name << ";\n";
        stream << "};";
    }
    return code;
}

QString
ShaderDefinition::shaderCode() const
{
    return p->d.shaderCode;
}

void
ShaderDefinition::setShaderCode(const QString& shaderCode)
{
    if (p->d.shaderCode != shaderCode) {
        p.detach();
        p->d.shaderCode = shaderCode;
    }
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

}  // namespace flipman::sdk::render
