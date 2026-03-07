// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class FpsPrivate;

/**
 * @class Fps
 * @brief Rational frame rate descriptor.
 */
class FLIPMANSDK_EXPORT Fps {
public:
    /**
     * @brief Constructs an invalid Fps.
     */
    Fps();

    /**
     * @brief Constructs an Fps from numerator and denominator.
     */
    explicit Fps(qint32 numerator, qint32 denominator, bool drop_frame = false);

    /**
     * @brief Copy constructor.
     */
    Fps(const Fps& other);

    /**
     * @brief Destroys the Fps.
     */
    ~Fps();

    /** @name Status and Properties */
    ///@{

    /**
     * @brief Returns true if drop-frame is enabled.
     */
    bool dropFrame() const;

    /**
     * @brief Returns the numerator.
     */
    qint64 numerator() const;

    /**
     * @brief Returns the denominator.
     */
    qint32 denominator() const;

    /**
     * @brief Returns true if valid.
     */
    bool isValid() const;

    /**
     * @brief Resets to invalid state.
     */
    void reset();

    ///@}

    /** @name Conversion and Math */
    ///@{

    /**
     * @brief Returns the integer frame quanta.
     */
    qint16 frameQuanta() const;

    /**
     * @brief Returns the frame scale.
     */
    qint32 frameScale() const;

    /**
     * @brief Returns the frame rate as a real value.
     */
    qreal real() const;

    /**
     * @brief Returns the duration of one frame in seconds.
     */
    qreal seconds() const;

    /**
     * @brief Converts a frame value to another Fps.
     */
    qreal toFps(qint64 frame, const Fps& other) const;

    /**
     * @brief Returns a string representation.
     */
    QString toString() const;

    ///@}

    /** @name Setters */
    ///@{

    /**
     * @brief Sets the numerator.
     */
    void setNumerator(qint32 numerator);

    /**
     * @brief Sets the denominator.
     */
    void setDenominator(qint32 denominator);

    /**
     * @brief Enables or disables drop-frame.
     */
    void setDropFrame(bool dropframe);

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    Fps& operator=(const Fps& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const Fps& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const Fps& other) const;

    /**
     * @brief Strict ordering operator.
     */
    bool operator<(const Fps& other) const;

    /**
     * @brief Greater-than operator.
     */
    bool operator>(const Fps& other) const;

    /**
     * @brief Less-than or equal operator.
     */
    bool operator<=(const Fps& other) const;

    /**
     * @brief Greater-than or equal operator.
     */
    bool operator>=(const Fps& other) const;

    /**
     * @brief Returns the frame rate as double.
     */
    operator double() const;

    ///@}

    /** @name Common Industry Rates */
    ///@{

    /**
     * @brief Guesses a rational Fps from a real value.
     */
    static Fps guess(qreal fps);

    static Fps fps23_976();
    static Fps fps24();
    static Fps fps25();
    static Fps fps29_97();
    static Fps fps30();
    static Fps fps47_952();
    static Fps fps48();
    static Fps fps50();
    static Fps fps59_94();
    static Fps fps60();

    ///@}

    /**
     * @brief Converts a frame value between two Fps.
     */
    static qint64 convert(quint64 value, const Fps& from, const Fps& to);

private:
    QExplicitlySharedDataPointer<FpsPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Fps)
