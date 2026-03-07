// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/metadata.h>
#include <flipmansdk/render/shaderdefinition.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::render {

class ImageEffectPrivate;

/**
 * @class ImageEffect
 * @brief Describes a render effect with an associated ShaderDefinition.
 *
 * ImageEffect stores effect identity and a prepared ShaderDefinition.
 * It does not perform composition or shader compilation.
 *
 * Uses implicit sharing for inexpensive copies.
 */
class FLIPMANSDK_EXPORT ImageEffect {
public:
    /**
     * @brief Constructs an empty effect.
     */
    ImageEffect();

    /**
     * @brief Copy constructor.
     */
    ImageEffect(const ImageEffect& other);

    /**
     * @brief Destroys the effect.
     */
    ~ImageEffect();

    /** @name Identity */
    ///@{

    /**
     * @brief Returns the display name of the effect.
     */
    QString name() const;

    /**
     * @brief Sets the display name of the effect.
     */
    void setName(const QString& name);

    /**
     * @brief Returns the logical group of the effect.
     *
     * Used for UI categorization.
     */
    QString group() const;

    /**
     * @brief Sets the logical group of the effect.
     */
    void setGroup(const QString& group);

    ///@}

    /** @name Shader Definition */
    ///@{

    /**
     * @brief Returns the associated ShaderDefinition.
     */
    ShaderDefinition shaderDefinition() const;

    /**
     * @brief Sets the associated ShaderDefinition.
     */
    void setShaderDefinition(const ShaderDefinition& definition);

    ///@}

    /** @name Status */
    ///@{

    /**
     * @brief Returns the effect error, if any.
     */
    core::Error error() const;

    /**
     * @brief Returns true if the effect is valid.
     */
    bool isValid() const;

    ///@}

    /** @name Management */
    ///@{

    /**
     * @brief Resets the effect to its default state.
     */
    void reset();

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator.
     */
    ImageEffect& operator=(const ImageEffect& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const ImageEffect& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const ImageEffect& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<ImageEffectPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::ImageEffect)
