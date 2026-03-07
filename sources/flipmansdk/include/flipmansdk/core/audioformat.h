// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::core {

class AudioFormatPrivate;

/**
 * @class AudioFormat
 * @brief Explicitly shared description of audio stream format.
 *
 * Defines sample rate, channel layout, and sample representation.
 */
class FLIPMANSDK_EXPORT AudioFormat {
public:
    /**
     * @brief Constructs an empty AudioFormat.
     */
    AudioFormat();

    /**
     * @brief Copy constructor.
     */
    AudioFormat(const AudioFormat& other);

    /**
     * @brief Destroys the AudioFormat.
     */
    ~AudioFormat();

    /**
     * @brief Returns true if the format contains valid parameters.
     */
    bool isValid() const;

    /**
     * @brief Resets the format to an empty state.
     */
    void reset();

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy of the shared data.
     */
    AudioFormat& operator=(const AudioFormat& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const AudioFormat& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const AudioFormat& other) const;

    /**
     * @brief Strict ordering operator.
     *
     * Provides deterministic ordering for use in associative containers.
     */
    bool operator<(const AudioFormat& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<AudioFormatPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::AudioFormat)
