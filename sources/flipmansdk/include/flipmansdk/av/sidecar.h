// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class SideCarPrivate;

/**
 * @class SideCar
 * @brief Shared external metadata container.
 */
class FLIPMANSDK_EXPORT SideCar {
public:
    /**
     * @brief Constructs an invalid SideCar.
     */
    SideCar();

    /**
     * @brief Copy constructor.
     */
    SideCar(const SideCar& other);

    /**
     * @brief Destroys the SideCar.
     */
    ~SideCar();

    /** @name Status */
    ///@{

    /**
     * @brief Returns true if valid.
     */
    bool isValid() const;

    /**
     * @brief Resets to invalid state.
     */
    void reset();

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    SideCar& operator=(const SideCar& other);

    ///@}

private:
    QExplicitlySharedDataPointer<SideCarPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::SideCar)
