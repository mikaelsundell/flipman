// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/fps.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class TimePrivate;

/**
 * @class Time
 * @brief High-precision temporal representation using rational ticks and timescales.
 *
 * The Time class handles frame-accurate timing by representing time as a count of
 * units (ticks) over a resolution (timescale). This prevents the rounding errors
 * common with floating-point seconds, making it suitable for professional video
 * editing and playback.
 * * @note This class utilizes implicit sharing (Copy-on-Write) via
 * QExplicitlySharedDataPointer for efficient passing and memory management.
 */
class FLIPMANSDK_EXPORT Time {
public:
    /** @name Constructors */
    ///@{
    /**
     * @brief Constructs an invalid Time object.
     */
    Time();

    /**
     * @brief Constructs Time using raw tick values.
     * @param ticks Total number of units.
     * @param timescale The resolution of the ticks (e.g., 24000 for 24fps base).
     * @param fps The associated frame rate for frame-based conversions.
     */
    Time(qint64 ticks, qint32 timesScale, const Fps& fps);

    /**
     * @brief Constructs Time from a specific frame number.
     */
    Time(qint64 frame, const Fps& fps);

    /**
     * @brief Constructs Time from floating-point seconds.
     */
    Time(qreal seconds, const Fps& fps);

    /**
     * @brief Constructs a copy of Time but with a new tick value.
     */
    Time(const Time& other, qint64 ticks);

    /**
     * @brief Constructs a copy of Time but converted to a new frame rate.
     */
    Time(const Time& other, const Fps& fps);

    /**
     * @brief Copy constructor. Shallow copy due to implicit sharing.
     */
    Time(const Time& other);
    ///@}

    virtual ~Time();

    /** @name Status and Validation */
    ///@{
    bool isValid() const;
    void reset();
    ///@}

    /** @name Properties */
    ///@{
    Fps fps() const;
    qint64 ticks() const;
    qint32 timeScale() const;

    /**
     * @brief Returns Ticks Per Frame (TPF) based on current timescale and FPS.
     */
    qint64 tpf() const;
    ///@}

    /** @name Conversions and Calculations */
    ///@{
    /**
     * @brief Maps a specific frame to a tick value in the current timescale.
     */
    qint64 ticks(qint64 frame) const;

    /**
     * @brief Calculates the frame number corresponding to a tick value.
     */
    qint64 frame(qint64 ticks) const;

    /**
     * @brief Returns the total count of frames represented by the current ticks.
     */
    qint64 frames() const;

    /**
     * @brief Returns the index of the last full frame.
     */
    qint64 lastFrame() const;

    /**
     * @brief Aligns a tick value to the nearest frame boundary.
     */
    qint64 align(qint64 ticks) const;

    /**
     * @brief Returns the duration in seconds as a real number.
     */
    qreal seconds() const;
    ///@}

    /** @name String Formatting */
    ///@{
    /**
     * @brief Formats a specific tick value as a human-readable string.
     */
    QString toString(qint64 ticks) const;

    /**
     * @brief Formats the current time as a human-readable string.
     */
    QString toString() const;
    ///@}

    /** @name Setters */
    ///@{
    void setTicks(qint64 ticks);
    void setTimeScale(qint32 timescale);
    void setFps(const Fps& fps);
    ///@}

    /** @name Operators */
    ///@{
    Time& operator=(const Time& other);
    bool operator==(const Time& other) const;
    bool operator!=(const Time& other) const;
    bool operator<(const Time& other) const;
    bool operator>(const Time& other) const;
    bool operator<=(const Time& other) const;
    bool operator>=(const Time& other) const;
    Time operator+(const Time& other) const;
    Time operator-(const Time& other) const;

    /**
     * @brief Casts the time to a double (seconds) for convenience in math expressions.
     */
    operator double() const;
    ///@}

    /** @name Static Conversion Utilities */
    ///@{
    /**
     * @brief Converts a Time object to a new frame rate.
     */
    static Time convert(const Time& time, const Fps& to);

    /**
     * @brief Changes the underlying timescale of a Time object.
     * @param timescale The new resolution (default 24000).
     */
    static Time convert(const Time& time, qint32 timeScale = 24000);
    ///@}

private:
    QExplicitlySharedDataPointer<TimePrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Time)
