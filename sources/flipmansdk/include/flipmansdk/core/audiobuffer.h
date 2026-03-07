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
 * @brief Explicitly shared container for audio sample data.
 *
 * Provides value semantics with copy-on-write behavior.
 */
class FLIPMANSDK_EXPORT AudioBuffer {
public:
    /**
     * @brief Constructs an empty AudioBuffer.
     */
    AudioBuffer();

    /**
     * @brief Copy constructor.
     */
    AudioBuffer(const AudioBuffer& other);

    /**
     * @brief Destroys the AudioBuffer.
     */
    ~AudioBuffer();

    /**
     * @brief Returns the audio format.
     */
    AudioFormat audioFormat() const;

    /**
     * @brief Detaches shared data (copy-on-write).
     */
    void detach();

    /**
     * @brief Returns true if the buffer contains valid data.
     */
    bool isValid() const;

    /**
     * @brief Resets the buffer to an empty state.
     */
    void reset();

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy of the shared data.
     */
    AudioBuffer& operator=(const AudioBuffer& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const AudioBuffer& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const AudioBuffer& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<AudioBufferPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::AudioBuffer)
