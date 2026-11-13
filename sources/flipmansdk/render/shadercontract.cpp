// SPDX-License-Identifier: BSD-3-Clause

#include <flipmansdk/render/shadercontract.h>

namespace flipman::sdk::render {

class ShaderContractPrivate : public QSharedData {
public:
    ShaderContractPrivate();
    ~ShaderContractPrivate();

public:
    struct Data {
        core::Error error;
    };
    Data d;
};

ShaderContractPrivate::ShaderContractPrivate() {}

ShaderContractPrivate::~ShaderContractPrivate() {}

ShaderContract::ShaderContract()
    : p(new ShaderContractPrivate)
{}

ShaderContract::~ShaderContract() = default;

QString
ShaderContract::name(Type type)
{
    switch (type) {
    case Type::Effect: return "Effect";
    case Type::Idt: return "idt";
    case Type::Odt: return "odt";
    }
    return {};
}

QString
ShaderContract::signature(Type type)
{
    switch (type) {
    case Type::Effect: return "vec4 effect(vec4 color, ivec2 pixel, ivec2 size)";
    case Type::Idt: return "vec4 idt(vec4 color)";
    case Type::Odt: return "vec4 odt(vec4 color)";
    }
    return {};
}

QString
ShaderContract::call(Type type)
{
    switch (type) {
    case Type::Effect: return "color = effect(color, pixel, size);";
    case Type::Idt: return "color = idt(color);";
    case Type::Odt: return "color = odt(color);";
    }
    return {};
}

QString
ShaderContract::entryFunction(Type type)
{
    switch (type) {
    case Type::Effect: return "effect";
    case Type::Idt: return "idt";
    case Type::Odt: return "odt";
    }
    return {};
}

bool
ShaderContract::validate(Type type, const QVector<ShaderFunction>& functions)
{
    p->d.error = core::Error();
    QString returnType;
    QString functionName;
    QStringList parameters;

    switch (type) {
    case Type::Effect:
        returnType = "vec4";
        functionName = "effect";
        parameters = { "vec4", "ivec2", "ivec2" };
        break;

    case Type::Idt:
        returnType = "vec4";
        functionName = "idt";
        parameters = { "vec4" };
        break;

    case Type::Odt:
        returnType = "vec4";
        functionName = "odt";
        parameters = { "vec4" };
        break;
    }

    const QString expected = QString("%1 %2(%3)").arg(returnType).arg(functionName).arg(parameters.join(", "));
    bool foundFunction = false;
    for (const ShaderFunction& fn : functions) {
        if (fn.name != functionName)
            continue;
        foundFunction = true;
        if (fn.returnType != returnType) {
            p->d.error = core::Error("shadercontract", QString("Shader entry function '%1' has incorrect return type "
                                                               "(expected %2, found %3).")
                                                           .arg(functionName, returnType, fn.returnType));
            continue;
        }

        const bool floatPixelEffect = type == Type::Effect
                                      && fn.parameterTypes == QStringList({ "vec4", "vec2", "vec2" });
        if (fn.parameterTypes != parameters && !floatPixelEffect) {
            p->d.error = core::Error("shadercontract",
                                     QString("Shader entry function '%1' has incorrect parameters "
                                             "(expected (%2), found (%3)).")
                                         .arg(functionName, parameters.join(", "), fn.parameterTypes.join(", ")));
            continue;
        }

        p->d.error = core::Error();
        return true;
    }

    if (!foundFunction) {
        p->d.error = core::Error("shadercontract", QString("Missing required shader entry function: %1").arg(expected));
    }

    return false;
}

core::Error
ShaderContract::error() const
{
    return p->d.error;
}

bool
ShaderContract::isValid() const
{
    return !p->d.error.hasError();
}

}  // namespace flipman::sdk::render
