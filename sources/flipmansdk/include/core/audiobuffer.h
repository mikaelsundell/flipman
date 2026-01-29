// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <core/audioformat.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QObject>

namespace flipman::sdk::core {

class AudioBufferPrivate;

/**
 * @class AudioBuffer
 * @brief Manages a container for audio sample data using explicit sharing.
 * * Provides a lightweight handle to audio data, allowing for efficient passing
 * and assignment. Memory is only duplicated when explicitly detached or modified.
 */
class FLIPMANSDK_EXPORT AudioBuffer {
    Q_GADGET
public:
    /// Constructs an empty, invalid audio buffer.
    AudioBuffer();

    /// Copy constructor (shallow copy via shared data pointer).
    AudioBuffer(const AudioBuffer& other);

    virtual ~AudioBuffer();

    /// Returns the format (channels, sample rate, bit depth) of the audio data.
    AudioFormat audioFormat() const;

    /**
     * @brief Creates a deep copy of the underlying data if it is shared.
     * Use this before performing write operations on the buffer.
     */
    void detach();

    /// Returns true if the buffer contains valid data and a valid format.
    bool isValid() const;

    /// Clears the buffer data and resets the format.
    void reset();

    AudioBuffer& operator=(const AudioBuffer& other);
    bool operator==(const AudioBuffer& other) const;
    bool operator!=(const AudioBuffer& other) const;

private:
    QExplicitlySharedDataPointer<AudioBufferPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::AudioBuffer)
