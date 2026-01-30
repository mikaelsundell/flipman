// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class SmpteTimePrivate;

/**
 * @class SmpteTime
 * @brief Represents time in industry-standard SMPTE (HH:MM:SS:FF) format.
 *
 * SmpteTime provides tools to convert raw temporal data into professional
 * timecode. It supports various frame rates, drop-frame math, and high-precision
 * subframe addressing.
 *
 * @note This class uses implicit sharing via QExplicitlySharedDataPointer,
 * making it efficient to pass through the UI and rendering pipelines.
 */
class FLIPMANSDK_EXPORT SmpteTime {
public:
    /**
     * @brief Constructs an empty SmpteTime object.
     */
    SmpteTime();

    /**
     * @brief Constructs SmpteTime from a generic Time object.
     * @param time The source time to convert.
     */
    explicit SmpteTime(const Time& time);

    /**
     * @brief Copy constructor. Performs a shallow copy of the shared data.
     */
    SmpteTime(const SmpteTime& other);

    /**
     * @brief Destroys the SmpteTime object.
     * @note Required for the PIMPL pattern to safely delete SmpteTimePrivate.
     */
    ~SmpteTime();

    /** @name Timecode Components */
    ///@{
    /**
     * @brief Returns the total running count of temporal units.
     */
    quint32 counter() const;

    /**
     * @brief Returns the hour component.
     */
    qint16 hours() const;

    /**
     * @brief Returns the minute component.
     */
    qint16 minutes() const;

    /**
     * @brief Returns the second component.
     */
    qint16 seconds() const;

    /**
     * @brief Returns the frame component based on the associated FPS.
     */
    qint16 frames() const;
    ///@}



    /** @name Precision and Frames */
    ///@{
    /**
     * @brief Returns the subframe position, used for audio-sync precision.
     */
    qint16 subFrames() const;

    /**
     * @brief Returns the scale for subframes (e.g., 80 or 100 divisions per frame).
     */
    qint16 subframeDivisor() const;

    /**
     * @brief Returns the total absolute frame number.
     */
    qint64 frame() const;

    /**
     * @brief Converts the current SMPTE breakdown back into a generic Time object.
     */
    Time time() const;
    ///@}

    /** @name Formatting Options */
    ///@{
    /**
     * @brief Returns true if the timecode supports negative values.
     */
    bool negatives() const;

    /**
     * @brief Returns true if hours are allowed to exceed the standard 24-hour clamp.
     */
    bool fullhours() const;

    /**
     * @brief Returns a standard timecode string (e.g., "01:00:04:05").
     * Uses ':' for non-drop and ';' for drop-frame FPS.
     */
    QString toString() const;
    ///@}

    /** @name Status and Validation */
    ///@{
    /**
     * @brief Returns true if the timecode is initialized and valid.
     */
    bool isValid() const;

    /**
     * @brief Resets the timecode to an uninitialized state.
     */
    void reset();
    ///@}

    /** @name Setters */
    ///@{
    void setTime(const Time& time);
    void setNegatives(bool negatives);
    void setFullHours(bool fullhours);
    ///@}

    /** @name Math Operators */
    ///@{
    SmpteTime& operator=(const SmpteTime& other);
    bool operator==(const SmpteTime& other) const;
    bool operator!=(const SmpteTime& other) const;
    bool operator<(const SmpteTime& other) const;
    bool operator>(const SmpteTime& other) const;
    bool operator<=(const SmpteTime& other) const;
    bool operator>=(const SmpteTime& other) const;
    SmpteTime operator+(const SmpteTime& other) const;
    SmpteTime operator-(const SmpteTime& other) const;
    ///@}

    /** @name Conversion Utilities */
    ///@{
    /**
     * @brief Maps a frame count from one frame rate to another.
     */
    static qint64 convert(quint64 frame, const Fps& from, const Fps& to);

    /**
     * @brief Converts between frame count and drop-frame timecode indices.
     */
    static qint64 convert(quint64 frame, const Fps& fps, bool reverse = false);
    ///@}

private:
    QExplicitlySharedDataPointer<SmpteTimePrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::SmpteTime)
