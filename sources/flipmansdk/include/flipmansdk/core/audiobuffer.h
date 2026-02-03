// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/audioformat.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::core {

class AudioBufferPrivate;

/**
 * @class AudioBuffer
 * @brief Manages a container for audio sample data using explicit sharing.
 * * Provides a lightweight handle to audio data, allowing for efficient passing
 * and assignment. Memory is only duplicated when explicitly detached or modified.
 */
class FLIPMANSDK_EXPORT AudioBuffer {
public:
    /**
     * @brief Constructs an empty, invalid audio buffer.
     */
    AudioBuffer();

    /**
     * @brief Copy constructor. Performs a shallow copy via shared data pointer.
     */
    AudioBuffer(const AudioBuffer& other);

    /**
     * @brief Destroys the audio buffer.
     * @note Required for the PIMPL pattern to safely delete AudioBufferPrivate.
     */
    ~AudioBuffer();

    /**
     * @brief Returns the format (channels, sample rate, bit depth) of the audio data.
     */
    AudioFormat audioFormat() const;

    /**
     * @brief Creates a deep copy of the underlying data if it is shared.
     * @note Use this before performing write operations on the buffer to ensure
     * thread-safe mutation.
     */
    void detach();

    /**
     * @brief Returns true if the buffer contains valid data and a valid format.
     */
    bool isValid() const;

    /**
     * @brief Clears the buffer data and resets the format.
     */
    void reset();

    /** @name Operators */
    ///@{
    AudioBuffer& operator=(const AudioBuffer& other);
    bool operator==(const AudioBuffer& other) const;
    bool operator!=(const AudioBuffer& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<AudioBufferPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::AudioBuffer)
