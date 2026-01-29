// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <av/time.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class TimeRangePrivate;

/**
 * @class TimeRange
 * @brief Represents a continuous segment of time defined by a start point and a duration.
 *
 * The TimeRange class is a fundamental building block for timeline organization.
 * It provides geometric-like operations for time, such as intersection testing and
 * value clamping (bounding).
 * * @note This class utilizes implicit sharing via QExplicitlySharedDataPointer
 * for high-performance copying and thread-safe data access.
 */
class FLIPMANSDK_EXPORT TimeRange {
public:
    /**
     * @brief Constructs an invalid TimeRange.
     */
    TimeRange();

    /**
     * @brief Constructs a TimeRange with a specific start and duration.
     * @param start The anchor point in time.
     * @param duration The length of the range.
     */
    TimeRange(Time start, Time duration);

    /**
     * @brief Copy constructor. Performs a shallow copy of the shared data.
     */
    TimeRange(const TimeRange& other);

    virtual ~TimeRange();

    /** @name Temporal Properties */
    ///@{
    /**
     * @brief Returns the start point of the range.
     */
    Time start() const;

    /**
     * @brief Returns the length of the range.
     */
    Time duration() const;

    /**
     * @brief Returns the calculated end point (start + duration).
     */
    Time end() const;
    ///@}

    /** @name Logic and Clamping */
    ///@{
    /**
     * @brief Clamps a given time value so it falls within this range.
     */
    Time bound(const Time& time);

    /**
     * @brief Clamps a given time value within this range, with optional looping logic.
     * @param time The time to evaluate.
     * @param loop If true, values exceeding 'end' wrap back to 'start'.
     */
    Time bound(const Time& time, bool loop = false);

    /**
     * @brief Checks if this range overlaps with another range.
     */
    bool intersects(const TimeRange& other) const;

    /**
     * @brief Returns a human-readable representation of the range (e.g., "[start - end]").
     */
    QString toString() const;
    ///@}

    /** @name Setters */
    ///@{
    void setStart(Time start);
    void setDuration(Time duration);
    ///@}

    /** @name Status and Validation */
    ///@{
    bool isValid() const;
    void reset();
    ///@}

    /** @name Operators */
    ///@{
    TimeRange& operator=(const TimeRange& other);
    bool operator==(const TimeRange& other) const;
    bool operator!=(const TimeRange& other) const;
    ///@}

    /** @name Static Conversion Utilities */
    ///@{
    /**
     * @brief Converts the entire range to a new frame rate.
     */
    static TimeRange convert(const TimeRange& timerange, const Fps& to);

    /**
     * @brief Changes the underlying timescale of the range.
     */
    static TimeRange convert(const TimeRange& timerange, qint32 timescale = 24000);
    ///@}

private:
    QExplicitlySharedDataPointer<TimeRangePrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::TimeRange)
