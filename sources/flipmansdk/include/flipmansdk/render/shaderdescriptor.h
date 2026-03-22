// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QString>
#include <QVariant>
#include <QVector>

namespace flipman::sdk::render {

/**
 * @class ShaderDescriptor
 * @brief Container describing shader parameters.
 *
 * Stores parameters declared via @param directives and is used
 * to expose shader controls and generate uniform blocks.
 */
class ShaderDescriptor {
public:
    /**
     * @struct ShaderParameter
     * @brief Describes a parameter declared via @param.
     */
    struct ShaderParameter {
        /**
         * @brief Supported parameter types.
         */
        enum class Type { Float, Int, Bool, Vec2, Vec3, Vec4 };

        QString name;             ///< Parameter name.
        Type type = Type::Float;  ///< Parameter type.

        QVariant defaultValue;  ///< Default value.
        QVariant minValue;      ///< Optional minimum value.
        QVariant maxValue;      ///< Optional maximum value.

        QString label;  ///< Optional UI label.
        QString group;  ///< Optional UI group.
    };

public:
    ShaderDescriptor() = default;

    /**
     * @brief Returns index of parameter by name.
     *
     * @param name Parameter name.
     * @return Parameter index or -1 if not found.
     */
    int indexOf(const QString& name) const
    {
        for (int i = 0; i < parameters.size(); ++i) {
            if (parameters[i].name == name)
                return i;
        }
        return -1;
    }

    /**
     * @brief Returns true if parameter exists.
     */
    bool contains(const QString& name) const { return indexOf(name) >= 0; }

public:
    QVector<ShaderParameter> parameters;  ///< Parameter list.
};

}  // namespace flipman::sdk::render
