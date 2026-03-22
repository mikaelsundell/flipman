// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QString>
#include <QStringList>

namespace flipman::sdk::render {

/**
 * @class ShaderFunction
 * @brief Describes a parsed GLSL function signature.
 *
 * Represents a function discovered during shader parsing.
 * Only the signature is stored, not the function body.
 *
 * Used for shader contract validation and reflection.
 */
class ShaderFunction {
public:
    /**
     * @brief Constructs an empty ShaderFunction.
     */
    ShaderFunction() = default;

    /**
     * @brief Constructs a ShaderFunction with signature information.
     *
     * @param returnType GLSL return type.
     * @param name Function name.
     * @param parameters Ordered list of parameter types.
     */
    ShaderFunction(QString returnType, QString name, QStringList parameters = {})
        : returnType(std::move(returnType))
        , name(std::move(name))
        , parameterTypes(std::move(parameters))
    {}

    /**
     * @brief Checks whether this function matches a given signature.
     *
     * @param ret Expected return type.
     * @param fn Expected function name.
     * @param params Expected parameter types.
     *
     * @return True if the signature matches.
     */
    bool matches(const QString& ret, const QString& fn, const QStringList& params) const
    {
        return returnType == ret && name == fn && parameterTypes == params;
    }

public:
    QString returnType;
    QString name;
    QStringList parameterTypes;
};

}  // namespace flipman::sdk::render
