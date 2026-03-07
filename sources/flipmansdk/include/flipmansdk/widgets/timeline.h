// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/flipmansdk.h>

#include <QScopedPointer>
#include <QWidget>

namespace flipman::sdk::widgets {

class TimelinePrivate;

/**
 * @class Timeline
 * @brief Widget for navigating temporal media.
 *
 * Provides frame-accurate scrubbing and range visualization.
 */
class FLIPMANSDK_EXPORT Timeline : public QWidget {
    Q_OBJECT
    Q_PROPERTY(av::Time time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(TimeCode timeCode READ timeCode WRITE setTimeCode)

public:
    /**
     * @enum TimeCode
     * @brief Display format for timeline values.
     */
    enum TimeCode {
        Frames,  ///< Displays absolute frame numbers.
        Time,    ///< Displays time in seconds.
        SMPTE    ///< Displays SMPTE timecode.
    };
    Q_ENUM(TimeCode)

public:
    /**
     * @brief Constructs a Timeline.
     *
     * @param parent Optional parent widget.
     */
    explicit Timeline(QWidget* parent = nullptr);

    /**
     * @brief Destroys the Timeline.
     */
    ~Timeline() override;

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the active time range.
     */
    av::TimeRange range() const;

    /**
     * @brief Returns the current playhead time.
     */
    av::Time time() const;

    /**
     * @brief Returns true if tracking is enabled.
     */
    bool tracking() const;

    /**
     * @brief Returns the current display format.
     */
    TimeCode timeCode() const;

    /**
     * @brief Returns the recommended widget size.
     */
    QSize sizeHint() const override;

    ///@}

public Q_SLOTS:

    /** @name Setters */
    ///@{

    /**
     * @brief Sets the active time range.
     */
    void setRange(const av::TimeRange& range);

    /**
     * @brief Sets the playhead time.
     */
    void setTime(const av::Time& time);

    /**
     * @brief Enables or disables tracking.
     */
    void setTracking(bool tracking);

    /**
     * @brief Sets the display format.
     */
    void setTimeCode(Timeline::TimeCode timeCode);

    ///@}

Q_SIGNALS:

    /** @name Notifications */
    ///@{

    /**
     * @brief Emitted when the playhead time changes.
     */
    void timeChanged(const av::Time& time);

    /**
     * @brief Emitted when the slider is moved.
     */
    void sliderMoved(const av::Time& time);

    /**
     * @brief Emitted when interaction begins.
     */
    void sliderPressed();

    /**
     * @brief Emitted when interaction ends.
     */
    void sliderReleased();

    ///@}

protected:
    /** @name Interaction & Rendering */
    ///@{

    /**
     * @brief Paint event handler.
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Mouse press event handler.
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Mouse move event handler.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Mouse release event handler.
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    ///@}

private:
    Q_DISABLE_COPY_MOVE(Timeline)
    QScopedPointer<TimelinePrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::widgets

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::widgets::Timeline*)
