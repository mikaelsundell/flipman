// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <core/error.h>
#include <core/parameters.h>

#include <QExplicitlySharedDataPointer>

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
     */
    virtual ~RenderEffect();

    /** @name Properties */
    ///@{
    /**
     * @brief Returns the key-value parameters used by this effect.
     */
    core::Parameters parameters() const;

    /**
     * @brief Returns the unique algorithm code or shader identifier for this effect.
     */
    QString code() const;
    ///@}

    /** @name Status */
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

    /** @name Setters */
    ///@{
    /**
     * @brief Updates the effect parameters.
     */
    void setParameters(const core::Parameters& parameters);

    /**
     * @brief Sets the algorithm or effect identifier.
     * @param code A string identifying the effect (e.g., "com.flipman.color_grade").
     */
    void setCode(const QString& code);
    ///@}

    /** @name Operators */
    ///@{
    RenderEffect& operator=(const RenderEffect& other);
    bool operator==(const RenderEffect& other) const;
    bool operator!=(const RenderEffect& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<RenderEffectPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::RenderEffect)
