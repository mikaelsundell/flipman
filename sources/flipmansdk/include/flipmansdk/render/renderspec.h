// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/rendersurface.h>
#include <QExplicitlySharedDataPointer>
#include <QMatrix4x4>
#include <QMetaType>
#include <QSize>
#include <QString>

namespace flipman::sdk::render {

class RenderSpecPrivate;

/**
 * @class RenderSpec
 * @brief A lightweight, render-ready description of an output pass.
 *
 * RenderSpec serves as the bridge between high-level output configuration and
 * the low-level RenderEngine. It describes where and how the already computed
 * render result should be written, including the destination surface, output
 * size, view transform, and optional output/calibration LUT.
 *
 * @note Because it uses QExplicitlySharedDataPointer, it is designed for cheap
 * value copies and safe handoffs between UI/output configuration and rendering
 * code.
 */
class FLIPMANSDK_EXPORT RenderSpec {
public:
    /**
     * @brief Constructs an empty RenderSpec.
     */
    RenderSpec();

    /**
     * @brief Copy constructor. Performs a shallow copy of the render spec data.
     */
    RenderSpec(const RenderSpec& other);

    /**
     * @brief Destroys the RenderSpec.
     * @note Required for the PIMPL pattern to safely delete RenderSpecPrivate.
     */
    ~RenderSpec();

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the output transform, for example viewer pan/zoom.
     */
    QMatrix4x4 view() const;

    /**
     * @brief Sets the output transform, for example viewer pan/zoom.
     */
    void setView(const QMatrix4x4& view);

    /**
     * @brief Returns the output size in pixels.
     */
    QSize size() const;

    /**
     * @brief Sets the output size in pixels.
     */
    void setSize(const QSize& size);

    /**
     * @brief Returns the optional output/calibration LUT path.
     */
    QString lut() const;

    /**
     * @brief Sets the optional output/calibration LUT path.
     */
    void setLut(const QString& lut);

    ///@}

    /** @name Status */
    ///@{

    /**
     * @brief Returns true if the output pass can be rendered.
     */
    bool isValid() const;

    /**
     * @brief Resets the render spec to an empty, uninitialized state.
     */
    void reset();

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy of the shared data.
     */
    RenderSpec& operator=(const RenderSpec& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const RenderSpec& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const RenderSpec& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<RenderSpecPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderSpec)
