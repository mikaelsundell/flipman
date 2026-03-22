// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>
#include <flipmansdk/render/shaderdescriptor.h>
#include <flipmansdk/render/shaderfunction.h>

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

    /**
     * @brief Returns parsed shader functions.
     *
     * Contains function signatures discovered during shader parsing.
     */
    QVector<ShaderFunction> functions() const;

    /**
     * @brief Sets parsed shader functions.
     *
     * Called by ShaderParser after extracting function signatures.
     */
    void setFunctions(const QVector<ShaderFunction>& functions);

    /** @name Code Fragments */
    ///@{

    /**
     * @brief Returns the generated uniform block for shader parameters.
     *
     * Generates a std140 uniform block from the ShaderDescriptor and binds it
     * to the specified @p binding index. The block name can be customized using
     * @p uniformName.
     *
     * The returned string contains only the GLSL uniform block declaration and
     * does not include any shader source code.
     */
    QString uniformBlock(int binding = 0, const QString& uniformName = "Params") const;

    /**
     * @brief Returns the stored GLSL shader source code.
     *
     * Contains the user-defined shader functions that will be injected into the
     * final shader during composition.
     */
    QString shaderCode() const;

    /**
     * @brief Sets shader function code.
     *
     * Defines the GLSL code block injected into the final shader.
     */
    void setShaderCode(const QString& shaderCode);

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
