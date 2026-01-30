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
 * @brief Represents a high-precision frame rate using rational numbers.
 *
 * The Fps class manages frame rates as fractions (numerator / denominator) to avoid
 * floating-point accumulation errors in long-form media. It supports industry-standard
 * rates, NTSC drop-frame logic, and provides utilities for frame-rate conversion.
 *
 * Because it uses QExplicitlySharedDataPointer, it is cheap to copy and can be
 * passed by value.
 */
class FLIPMANSDK_EXPORT Fps {
public:
    /**
     * @brief Constructs an invalid Fps object.
     */
    Fps();

    /**
     * @brief Constructs an Fps object from a rational fraction.
     * @param numerator The top of the fraction (e.g., 24000).
     * @param denominator The bottom of the fraction (e.g., 1001).
     * @param drop_frame Whether to use NTSC drop-frame timecode math.
     */
    explicit Fps(qint32 numerator, qint32 denominator, bool drop_frame = false);

    /**
     * @brief Copy constructor. Performs a shallow copy of the shared data.
     */
    Fps(const Fps& other);

    /**
     * @brief Destroys the Fps object.
     * @note Required for the PIMPL pattern to safely delete FpsPrivate.
     */
    ~Fps();

    /** @name Status and Properties */
    ///@{
    /**
     * @brief Returns true if NTSC drop-frame logic is enabled.
     */
    bool dropFrame() const;

    /**
     * @brief Returns the numerator of the frame rate fraction.
     */
    qint64 numerator() const;

    /**
     * @brief Returns the denominator of the frame rate fraction.
     */
    qint32 denominator() const;

    /**
     * @brief Returns true if the object contains a valid numerator and denominator.
     */
    bool isValid() const;

    /**
     * @brief Resets the Fps to an uninitialized state.
     */
    void reset();
    ///@}

    /** @name Conversion and Math */
    ///@{
    /**
     * @brief Returns the integer part of the frame rate (e.g., 24 for 23.976).
     */
    qint16 frameQuanta() const;

    /**
     * @brief Returns the scaled integer representation.
     */
    qint32 frameScale() const;

    /**
     * @brief Returns the frame rate as a real number (e.g., 23.976023...).
     */
    qreal real() const;

    /**
     * @brief Returns the duration of a single frame in seconds.
     */
    qreal seconds() const;

    /**
     * @brief Maps a frame number from this FPS to another FPS timeline.
     */
    qreal toFps(qint64 frame, const Fps& other) const;

    /**
     * @brief Returns a human-readable string (e.g., "23.976").
     */
    QString toString() const;
    ///@}



    /** @name Setters */
    ///@{
    void setNumerator(qint32 numerator);
    void setDenominator(qint32 denominator);
    void setDropFrame(bool dropframe);
    ///@}

    /** @name Operators */
    ///@{
    Fps& operator=(const Fps& other);
    bool operator==(const Fps& other) const;
    bool operator!=(const Fps& other) const;
    bool operator<(const Fps& other) const;
    bool operator>(const Fps& other) const;
    bool operator<=(const Fps& other) const;
    bool operator>=(const Fps& other) const;

    /**
     * @brief Convenience operator to return the frame rate as a double.
     */
    operator double() const;
    ///@}

    /** @name Common Industry Rates */
    ///@{
    /**
     * @brief Attempts to find the closest rational match for a given float value.
     */
    static Fps guess(qreal fps);

    static Fps fps23_976();  // 24000/1001
    static Fps fps24();      // 24/1
    static Fps fps25();      // 25/1 (PAL)
    static Fps fps29_97();   // 30000/1001
    static Fps fps30();      // 30/1
    static Fps fps47_952();
    static Fps fps48();
    static Fps fps50();
    static Fps fps59_94();
    static Fps fps60();
    ///@}

    /**
     * @brief Static utility to convert a frame count between two different timebases.
     */
    static qint64 convert(quint64 value, const Fps& from, const Fps& to);

private:
    QExplicitlySharedDataPointer<FpsPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Fps)
