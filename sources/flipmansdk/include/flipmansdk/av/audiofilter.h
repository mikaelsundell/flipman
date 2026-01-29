// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/parameters.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::av {

class AudioFilterPrivate;

/**
 * @class AudioFilter
 * @brief Defines a processing stage for audio data streams.
 * * AudioFilter encapsulates the parameters and logic required to modify audio buffers.
 * This can include gain adjustments, equalization, or dynamic range compression.
 * The filter uses a string-based 'code' identifier to map to specific backend
 * implementations (e.g., FFmpeg filters or internal DSP).
 * * @note As a shared data object, multiple Clips can share the same AudioFilter
 * instance to maintain synchronized processing settings.
 */
class FLIPMANSDK_EXPORT AudioFilter {
public:
    /**
     * @brief Constructs an empty AudioFilter.
     */
    AudioFilter();

    /**
     * @brief Copy constructor. Performs a shallow copy of the filter data.
     */
    AudioFilter(const AudioFilter& other);

    /**
     * @brief Destroys the AudioFilter.
     */
    virtual ~AudioFilter();

    /**
     * @brief Returns the collection of key-value parameters for this filter.
     */
    core::Parameters parameters() const;

    /**
     * @brief Returns the unique identifier/type code for the filter algorithm.
     */
    QString code() const;

    /**
     * @brief Returns any error encountered during filter configuration or execution.
     */
    core::Error error() const;

    /**
     * @brief Resets the filter to its default parameters and clears the code.
     */
    void reset();

    /** @name Setters */
    ///@{
    /**
     * @brief Sets the processing parameters.
     */
    void set_parameters(const core::Parameters& parameters);

    /**
     * @brief Sets the filter type or algorithm code.
     */
    void set_code(const QString& code);
    ///@}

    /** @name Operators */
    ///@{
    AudioFilter& operator=(const AudioFilter& other);
    bool operator==(const AudioFilter& other) const;
    bool operator!=(const AudioFilter& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<AudioFilterPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::AudioFilter)
