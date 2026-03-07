// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/time.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class TimeRangePrivate;

/**
 * @class TimeRange
 * @brief Continuous time segment descriptor.
 */
class FLIPMANSDK_EXPORT TimeRange {
public:
    /**
     * @brief Constructs an invalid TimeRange.
     */
    TimeRange();

    /**
     * @brief Constructs TimeRange from start and duration.
     */
    TimeRange(Time start, Time duration);

    /**
     * @brief Copy constructor.
     */
    TimeRange(const TimeRange& other);

    /**
     * @brief Destroys the TimeRange.
     */
    ~TimeRange();

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the start time.
     */
    Time start() const;

    /**
     * @brief Sets the start time.
     */
    void setStart(Time start);

    /**
     * @brief Returns the duration.
     */
    Time duration() const;

    /**
     * @brief Sets the duration.
     */
    void setDuration(Time duration);

    /**
     * @brief Returns the end time.
     */
    Time end() const;

    ///@}

    /** @name Logic */
    ///@{

    /**
     * @brief Bounds time to range.
     */
    Time bound(const Time& time);

    /**
     * @brief Bounds time with optional loop.
     */
    Time bound(const Time& time, bool loop = false);

    /**
     * @brief Returns true if intersects.
     */
    bool intersects(const TimeRange& other) const;

    /**
     * @brief Returns string representation.
     */
    QString toString() const;

    ///@}

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

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    TimeRange& operator=(const TimeRange& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const TimeRange& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const TimeRange& other) const;

    ///@}

    /** @name Static Utilities */
    ///@{

    /**
     * @brief Converts to a new Fps.
     */
    static TimeRange convert(const TimeRange& timerange, const Fps& to);

    /**
     * @brief Converts to a new timescale.
     */
    static TimeRange convert(const TimeRange& timerange, qint32 timescale = 24000);

    ///@}

private:
    QExplicitlySharedDataPointer<TimeRangePrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::TimeRange)
