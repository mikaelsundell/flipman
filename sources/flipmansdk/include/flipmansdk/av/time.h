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
 * @brief Rational time descriptor.
 */
class FLIPMANSDK_EXPORT Time {
public:
    /** @name Constructors */
    ///@{

    /**
     * @brief Constructs an invalid Time.
     */
    Time();

    /**
     * @brief Constructs Time from ticks and timescale.
     */
    Time(qint64 ticks, qint32 timeScale, const Fps& fps);

    /**
     * @brief Constructs Time from other with new ticks.
     */
    Time(const Time& other, qint64 ticks);

    /**
     * @brief Constructs Time from other with new Fps.
     */
    Time(const Time& other, const Fps& fps);

    /**
     * @brief Copy constructor.
     */
    Time(const Time& other);

    ///@}

    /**
     * @brief Destroys the Time.
     */
    ~Time();

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

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the associated Fps.
     */
    Fps fps() const;

    /**
     * @brief Returns the tick count.
     */
    qint64 ticks() const;

    /**
     * @brief Returns the timescale.
     */
    qint32 timeScale() const;

    /**
     * @brief Returns ticks per frame.
     */
    qint64 tpf() const;

    ///@}

    /** @name Conversion */
    ///@{

    /**
     * @brief Returns ticks for a frame.
     */
    qint64 ticks(qint64 frame) const;

    /**
     * @brief Returns frame for ticks.
     */
    qint64 frame(qint64 ticks) const;

    /**
     * @brief Returns total frames.
     */
    qint64 frames() const;

    /**
     * @brief Returns last full frame.
     */
    qint64 lastFrame() const;

    /**
     * @brief Aligns ticks to frame boundary.
     */
    qint64 align(qint64 ticks) const;

    /**
     * @brief Returns seconds.
     */
    qreal seconds() const;

    ///@}

    /** @name Formatting */
    ///@{

    /**
     * @brief Returns string for ticks.
     */
    QString toString(qint64 ticks) const;

    /**
     * @brief Returns string representation.
     */
    QString toString() const;

    ///@}

    /** @name Setters */
    ///@{

    /**
     * @brief Sets ticks.
     */
    void setTicks(qint64 ticks);

    /**
     * @brief Sets timescale.
     */
    void setTimeScale(qint32 timeScale);

    /**
     * @brief Sets Fps.
     */
    void setFps(const Fps& fps);

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    Time& operator=(const Time& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const Time& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const Time& other) const;

    /**
     * @brief Strict ordering operator.
     */
    bool operator<(const Time& other) const;

    /**
     * @brief Greater-than operator.
     */
    bool operator>(const Time& other) const;

    /**
     * @brief Less-than or equal operator.
     */
    bool operator<=(const Time& other) const;

    /**
     * @brief Greater-than or equal operator.
     */
    bool operator>=(const Time& other) const;

    /**
     * @brief Addition operator.
     */
    Time operator+(const Time& other) const;

    /**
     * @brief Subtraction operator.
     */
    Time operator-(const Time& other) const;

    /**
     * @brief Returns time as seconds.
     */
    operator double() const;

    ///@}

    /**
     * @brief Returns Time at frame zero for the given Fps.
     */
    static Time zero(const Fps& fps);

    /**
     * @brief Constructs Time from frame and Fps.
     */
    static Time fromFrames(qint64 frame, const Fps& fps);

    /**
     * @brief Constructs Time from seconds and Fps.
     */
    static Time fromSeconds(qreal seconds, const Fps& fps);

    /** @name Static Utilities */
    ///@{

    /**
     * @brief Converts to a new Fps.
     */
    static Time convert(const Time& time, const Fps& to);

    /**
     * @brief Converts to a new timescale.
     */
    static Time convert(const Time& time, qint32 timeScale = 24000);

    ///@}

private:
    QExplicitlySharedDataPointer<TimePrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Time)
