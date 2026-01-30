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
 * @brief Describes the technical layout of audio data.
 *
 * This class defines the properties of audio streams, such as sample rate,
 * channel count, and sample format (bit depth). It uses explicit data
 * sharing to remain lightweight when passed through signal/slot connections.
 */
class FLIPMANSDK_EXPORT AudioFormat {
public:
    /**
     * @brief Constructs an invalid audio format.
     */
    AudioFormat();

    /**
     * @brief Copy constructor. Performs a shallow copy of the shared data.
     */
    AudioFormat(const AudioFormat& other);

    /**
     * @brief Destroys the audio format.
     * @note Required for the PIMPL pattern to safely delete AudioFormatPrivate.
     */
    ~AudioFormat();

    /**
     * @brief Checks if the format is initialized with valid audio parameters.
     * @return true if the sample rate and channel count are greater than zero.
     */
    bool isValid() const;

    /**
     * @brief Resets the format to an uninitialized state.
     */
    void reset();

    /** @name Operators */
    ///@{
    AudioFormat& operator=(const AudioFormat& other);
    bool operator==(const AudioFormat& other) const;
    bool operator!=(const AudioFormat& other) const;
    bool operator<(const AudioFormat& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<AudioFormatPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::AudioFormat)
