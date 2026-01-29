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
    /// Constructs an invalid audio format.
    AudioFormat();

    /// Copy constructor (shallow copy).
    AudioFormat(const AudioFormat& format);

    virtual ~AudioFormat();

    /**
     * @brief Checks if the format is initialized with valid audio parameters.
     * @return true if the sample rate and channel count are greater than zero.
     */
    bool isValid() const;

    /// Resets the format to an uninitialized state.
    void reset();

    AudioFormat& operator=(const AudioFormat& other);
    bool operator==(const AudioFormat& other) const;
    bool operator!=(const AudioFormat& other) const;

private:
    QExplicitlySharedDataPointer<AudioFormatPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::AudioFormat)
