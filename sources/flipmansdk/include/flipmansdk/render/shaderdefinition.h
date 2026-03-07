// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

namespace flipman::sdk::render {

class ShaderDefinitionPrivate;

/**
 * @class ShaderDefinition
 * @brief Parsed shader artifact with parameter metadata.
 *
 * Holds processed effect code and extracted @param information.
 * Does not perform compilation or interact with QRhi.
 *
 * Uses implicit sharing for inexpensive copies.
 */
class FLIPMANSDK_EXPORT ShaderDefinition {
public:
    /**
     * @struct ShaderParameter
     * @brief Describes a parameter declared via @param.
     */
    struct ShaderParameter {
        enum class Type { Float, Int, Bool, Vec2, Vec3, Vec4 };
        QString name;
        Type type = Type::Float;
        QVariant defaultValue;
        QVariant minValue;
        QVariant maxValue;
        QString label;
        QString group;
    };

    /**
     * @struct ShaderDescriptor
     * @brief Container of shader parameters.
     */
    struct ShaderDescriptor {
        QVector<ShaderParameter> parameters;

        /**
         * @brief Returns index of parameter by name.
         */
        int indexOf(const QString& name) const;

        /**
         * @brief Returns true if parameter exists.
         */
        bool contains(const QString& name) const { return indexOf(name) >= 0; }
    };

public:
    /**
     * @brief Constructs an empty ShaderDefinition.
     */
    ShaderDefinition();

    /**
     * @brief Copy constructor. Performs a shallow copy of the layer data.
     */
    ShaderDefinition(const ShaderDefinition& other);

    /**
     * @brief Destroys the ShaderDefinition.
     * @note Required for the PIMPL pattern to safely delete ShaderDefinitionPrivate.
     */
    ~ShaderDefinition();

    /** @name Code Fragments */
    ///@{

    /**
     * @brief Returns generated uniform block code.
     */
    QString uniformBlock() const;

    /**
     * @brief Sets uniform block code.
     */
    void setUniformBlock(const QString& code);

    /**
     * @brief Returns processed shader code.
     */
    QString shaderCode() const;

    /**
     * @brief Sets processed shader code.
     */
    void setShaderCode(const QString& shaderCode);

    /**
     * @brief Returns effect apply snippet.
     */
    QString applyCode() const;

    /**
     * @brief Sets effect apply snippet.
     */
    void setApplyCode(const QString& code);

    ///@}

    /** @name Descriptor */
    ///@{

    /**
     * @brief Returns parameter descriptor.
     */
    const ShaderDescriptor& descriptor() const;

    /**
     * @brief Sets parameter descriptor.
     */
    void setDescriptor(const ShaderDescriptor& descriptor);

    ///@}

    /** @name Status */
    ///@{

    /**
     * @brief Returns generation error, if any.
     */
    core::Error error() const;

    /**
     * @brief Returns true if definition is valid.
     */
    bool isValid() const;

    ///@}

    /**
     * @brief Assignment operator.
     */
    ShaderDefinition& operator=(const ShaderDefinition& other);

private:
    QExplicitlySharedDataPointer<ShaderDefinitionPrivate> p;
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::ShaderDefinition)
