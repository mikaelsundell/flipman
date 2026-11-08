// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <QVector>
#include <flipmansdk/flipmansdk.h>

namespace flipman::sdk::render {

/**
 * @class ShaderDescriptor
 * @brief Container describing shader parameters and resources.
 *
 * Stores parameters declared via @param directives and is used to expose
 * shader controls, generate uniform blocks, and describe external resources
 * such as LUT textures.
 */
class ShaderDescriptor {
public:
    /**
     * @struct ShaderOption
     * @brief Describes one discrete UI option for a shader parameter.
     *
     * Used for parameters that should be represented as a combo box or
     * another discrete selector instead of a continuous slider.
     */
    struct ShaderOption {
        /**
         * @brief Display label shown in the UI.
         */
        QString label;

        /**
         * @brief Actual parameter value assigned when selected.
         */
        QVariant value;
    };

    /**
     * @struct ShaderParameter
     * @brief Describes one parameter or resource declared via @param.
     */
    struct ShaderParameter {
        /**
         * @enum Type
         * @brief Supported shader parameter/resource types.
         */
        enum class Type { Float, Int, Bool, Vec2, Vec3, Vec4, Lut };

        /**
         * @brief Parameter/resource name.
         */
        QString name;

        /**
         * @brief Parameter/resource type.
         */
        Type type = Type::Float;

        /**
         * @brief Current value.
         */
        QVariant value;

        /**
         * @brief Default value from the @param declaration.
         */
        QVariant defaultValue;

        /**
         * @brief Minimum value for numeric controls.
         */
        QVariant minValue;

        /**
         * @brief Maximum value for numeric controls.
         */
        QVariant maxValue;

        /**
         * @brief Optional discrete values for combo-style controls.
         */
        QVector<ShaderOption> options;

        /**
         * @brief Optional UI label.
         */
        QString label;

        /**
         * @brief Optional UI group.
         */
        QString group;

        /**
         * @brief Returns true if the parameter has discrete options.
         */
        bool hasOptions() const { return !options.isEmpty(); }

        /**
         * @brief Returns true if the parameter should be stored in the std140 uniform block.
         */
        bool isUniform() const { return type != Type::Lut; }

        /**
         * @brief Returns true if the parameter represents an external shader resource.
         */
        bool isResource() const { return type == Type::Lut; }

        /**
         * @brief Returns true if the parameter represents a LUT resource.
         */
        bool isLut() const { return type == Type::Lut; }
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

    /**
     * @brief Returns all parameters matching a specific type.
     */
    QList<ShaderParameter> parametersByType(ShaderParameter::Type type) const
    {
        QList<ShaderParameter> result;
        for (const ShaderParameter& parameter : parameters) {
            if (parameter.type == type)
                result.append(parameter);
        }
        return result;
    }

    /**
     * @brief Returns parameters that should be uploaded to the std140 uniform block.
     */
    QList<ShaderParameter> uniformParameters() const
    {
        QList<ShaderParameter> result;
        for (const ShaderParameter& parameter : parameters) {
            if (parameter.isUniform())
                result.append(parameter);
        }
        return result;
    }

    /**
     * @brief Returns parameters that describe external shader resources.
     */
    QList<ShaderParameter> resourceParameters() const
    {
        QList<ShaderParameter> result;
        for (const ShaderParameter& parameter : parameters) {
            if (parameter.isResource())
                result.append(parameter);
        }
        return result;
    }

    /**
     * @brief Returns LUT resource parameters.
     */
    QList<ShaderParameter> lutParameters() const { return parametersByType(ShaderParameter::Type::Lut); }

public:
    /**
     * @brief Parameter/resource list.
     */
    QList<ShaderParameter> parameters;
};

}  // namespace flipman::sdk::render
