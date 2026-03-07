// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/fps.h>

#include <QList>
#include <QScopedPointer>

namespace flipman::sdk::av {

class TimerPrivate;

/**
 * @class Timer
 * @brief High-precision playback timer.
 */
class FLIPMANSDK_EXPORT Timer {
public:
    /**
     * @brief Time units.
     */
    enum Unit { Nanos, Seconds, Minutes, Hours };

    /**
     * @brief Constructs a Timer.
     */
    Timer();

    /**
     * @brief Destroys the Timer.
     */
    ~Timer();

    /** @name Control */
    ///@{

    /**
     * @brief Starts the timer.
     */
    void start();

    /**
     * @brief Starts the timer with Fps.
     */
    void start(const Fps& fps);

    /**
     * @brief Stops the timer.
     */
    void stop();

    /**
     * @brief Restarts the timer.
     */
    void restart();

    /**
     * @brief Records a lap.
     */
    void lap();

    /**
     * @brief Returns true if next frame interval reached.
     */
    bool next(const Fps& fps);

    /**
     * @brief Waits until next interval.
     */
    void wait();

    /**
     * @brief Sleeps for milliseconds.
     */
    void sleep(quint64 msecs);

    ///@}

    /** @name Query */
    ///@{

    /**
     * @brief Returns elapsed nanoseconds.
     */
    quint64 elapsed() const;

    /**
     * @brief Returns recorded laps.
     */
    QList<quint64> laps() const;

    /**
     * @brief Returns true if running.
     */
    bool isValid() const;

    /**
     * @brief Resets the timer.
     */
    void reset();

    ///@}

    /** @name Utilities */
    ///@{

    /**
     * @brief Converts nanoseconds to unit.
     */
    static qreal convert(quint64 nano, Unit unit = Unit::Nanos);

    ///@}

private:
    Q_DISABLE_COPY_MOVE(Timer)
    QScopedPointer<TimerPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Timer*)
