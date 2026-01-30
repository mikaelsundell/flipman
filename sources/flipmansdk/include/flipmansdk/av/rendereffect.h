// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/parameters.h>
#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class RenderEffectPrivate;

/**
 * @class RenderEffect
 * @brief Defines a visual processing stage for image data within the render pipeline.
 *
 * RenderEffect encapsulates the logic and parameters for image manipulation, such as
 * color corrections, blurs, or custom shaders. It uses a unique 'code' identifier to
 * map to specific hardware (GPU/RHI) or software (CPU) implementations.
 *
 * @note Because it uses QExplicitlySharedDataPointer, it supports efficient
 * shallow copying, allowing multiple clips to share effect configurations.
 */
class FLIPMANSDK_EXPORT RenderEffect {
public:
    /**
     * @brief Constructs an empty RenderEffect.
     */
    RenderEffect();

    /**
     * @brief Copy constructor. Performs a shallow copy of the effect data.
     */
    RenderEffect(const RenderEffect& other);

    /**
     * @brief Destroys the RenderEffect.
     * @note Marked virtual to allow for specialized effect implementations.
     */
    virtual ~RenderEffect();

    /** @name Attributes */
    ///@{
    /**
     * @brief Returns the key-value parameters used by this effect.
     */
    core::Parameters parameters() const;

    /**
     * @brief Updates the effect parameters.
     */
    void setParameters(const core::Parameters& parameters);

    /**
     * @brief Returns the unique algorithm code or shader identifier for this effect.
     */
    QString code() const;

    /**
     * @brief Sets the algorithm or effect identifier.
     * @param code A string identifying the effect (e.g., "com.flipman.blur").
     */
    void setCode(const QString& code);
    ///@}



    /** @name Management */
    ///@{
    /**
     * @brief Returns any error associated with the effect configuration.
     */
    core::Error error() const;

    /**
     * @brief Resets the effect to default parameters and clears the code.
     */
    void reset();
    ///@}

    /** @name Operators */
    ///@{
    /**
     * @brief Assignment operator. Performs a shallow copy of the shared data.
     */
    RenderEffect& operator=(const RenderEffect& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const RenderEffect& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const RenderEffect& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<RenderEffectPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::RenderEffect)
