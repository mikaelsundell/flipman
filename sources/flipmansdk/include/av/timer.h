// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <av/fps.h>

#include <QList>
#include <QScopedPointer>

namespace flipman::sdk::av {

class TimerPrivate;

/**
 * @class Timer
 * @brief A high-precision monotonic timer for playback synchronization and profiling.
 * * The Timer class provides nanosecond-accurate timing utilities. It is primarily
 * used to maintain stable playback speeds by calculating frame intervals based on
 * Fps. It also supports "lap" timing, which is useful for profiling the performance
 * of the RenderEngine or MediaProcessor.
 */
class FLIPMANSDK_EXPORT Timer {
public:
    /**
     * @enum Unit
     * @brief Time units for conversion and reporting.
     */
    enum Unit {
        Nanos,    ///< Nanoseconds
        Seconds,  ///< Seconds
        Minutes,  ///< Minutes
        Hours     ///< Hours
    };

    /**
     * @brief Constructs a new Timer instance.
     */
    Timer();

    /**
     * @brief Destroys the Timer.
     */
    virtual ~Timer();

    /**
     * @brief Starts the timer.
     */
    void start();

    /**
     * @brief Starts the timer and internally calculates the interval based on FPS.
     * @param fps The frame rate to synchronize against.
     */
    void start(const Fps& fps);

    /**
     * @brief Stops the timer and records the final elapsed time.
     */
    void stop();

    /**
     * @brief Resets the elapsed time to zero and starts the timer immediately.
     */
    void restart();

    /**
     * @brief Captures the current elapsed time as a "lap" without stopping the timer.
     */
    void lap();

    /**
     * @brief Determines if enough time has elapsed to advance to the next frame.
     * * This is used in "real-time" playback loops to decide when to trigger a
     * new frame render based on the provided FPS.
     * @param fps The target playback frame rate.
     * @return true if the interval for the next frame has been reached.
     */
    bool next(const Fps& fps);

    /**
     * @brief Blocks the current thread until the next frame interval is reached.
     * Used to throttle playback loops to the correct speed.
     */
    void wait();

    /**
     * @brief Suspends the current thread for a specific duration.
     * @param msecs Duration in milliseconds.
     */
    void sleep(quint64 msecs);

    /**
     * @brief Returns the total nanoseconds elapsed since the timer started.
     */
    quint64 elapsed() const;

    /**
     * @brief Returns a list of all captured lap times in nanoseconds.
     */
    QList<quint64> laps() const;

    /**
     * @brief Returns true if the timer has been started and is running.
     */
    bool isValid() const;

    /**
     * @brief Stops the timer and clears all recorded data.
     */
    void reset();

    /**
     * @brief Static helper to convert nanoseconds into other time units.
     * @param nano The nanosecond value.
     * @param unit The target unit for the conversion.
     * @return The converted value as a real number.
     */
    static qreal convert(quint64 nano, Unit unit = Unit::Nanos);

private:
    Q_DISABLE_COPY_MOVE(Timer)
    QScopedPointer<TimerPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Timer*)
