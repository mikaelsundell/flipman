// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/time.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class SmpteTimePrivate;

/**
 * @class SmpteTime
 * @brief SMPTE timecode descriptor.
 */
class FLIPMANSDK_EXPORT SmpteTime {
public:
    /**
     * @brief Constructs an invalid SmpteTime.
     */
    SmpteTime();

    /**
     * @brief Constructs SmpteTime from Time.
     */
    explicit SmpteTime(const Time& time);

    /**
     * @brief Copy constructor.
     */
    SmpteTime(const SmpteTime& other);

    /**
     * @brief Destroys the SmpteTime.
     */
    ~SmpteTime();

    /** @name Timecode Components */
    ///@{

    /**
     * @brief Returns the counter value.
     */
    quint32 counter() const;

    /**
     * @brief Returns the hours component.
     */
    qint16 hours() const;

    /**
     * @brief Returns the minutes component.
     */
    qint16 minutes() const;

    /**
     * @brief Returns the seconds component.
     */
    qint16 seconds() const;

    /**
     * @brief Returns the frames component.
     */
    qint16 frames() const;

    ///@}

    /** @name Precision */
    ///@{

    /**
     * @brief Returns the subframe value.
     */
    qint16 subFrames() const;

    /**
     * @brief Returns the subframe divisor.
     */
    qint16 subframeDivisor() const;

    /**
     * @brief Returns the absolute frame number.
     */
    qint64 frame() const;

    /**
     * @brief Returns the corresponding Time.
     */
    Time time() const;

    ///@}

    /** @name Formatting */
    ///@{

    /**
     * @brief Returns true if negatives are enabled.
     */
    bool negatives() const;

    /**
     * @brief Returns true if full hours are enabled.
     */
    bool fullhours() const;

    /**
     * @brief Returns a string representation.
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

    /** @name Setters */
    ///@{

    /**
     * @brief Sets the Time value.
     */
    void setTime(const Time& time);

    /**
     * @brief Enables or disables negatives.
     */
    void setNegatives(bool negatives);

    /**
     * @brief Enables or disables full hours.
     */
    void setFullHours(bool fullhours);

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    SmpteTime& operator=(const SmpteTime& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const SmpteTime& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const SmpteTime& other) const;

    /**
     * @brief Strict ordering operator.
     */
    bool operator<(const SmpteTime& other) const;

    /**
     * @brief Greater-than operator.
     */
    bool operator>(const SmpteTime& other) const;

    /**
     * @brief Less-than or equal operator.
     */
    bool operator<=(const SmpteTime& other) const;

    /**
     * @brief Greater-than or equal operator.
     */
    bool operator>=(const SmpteTime& other) const;

    /**
     * @brief Addition operator.
     */
    SmpteTime operator+(const SmpteTime& other) const;

    /**
     * @brief Subtraction operator.
     */
    SmpteTime operator-(const SmpteTime& other) const;

    ///@}

    /** @name Conversion Utilities */
    ///@{

    /**
     * @brief Converts frame count between frame rates.
     */
    static qint64 convert(quint64 frame, const Fps& from, const Fps& to);

    /**
     * @brief Converts frame count using drop-frame logic.
     */
    static qint64 convert(quint64 frame, const Fps& fps, bool reverse = false);

    ///@}

private:
    QExplicitlySharedDataPointer<SmpteTimePrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::SmpteTime)
