// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <av/time.h>
#include <av/timerange.h>

#include <QScopedPointer>
#include <QWidget>

namespace flipman::sdk::widgets {

class TimelineWidgetPrivate;

/**
 * @class TimelineWidget
 * @brief A high-performance transport control for navigating temporal media.
 *
 * The TimelineWidget provides a visual interface for navigating through frame ranges,
 * supporting frame-accurate scrubbing, range visualization, and multiple timecode
 * display formats. It is designed for tight integration with the Flipman playback engine.
 */
class FLIPMANSDK_EXPORT TimelineWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(av::Time time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(TimeCode timeCode READ timeCode WRITE setTimeCode)

public:
    /**
     * @enum TimeCode
     * @brief Defines the temporal formatting used for the timeline markers and display.
     */
    enum TimeCode {
        Frames,  ///< Displays absolute frame numbers.
        Time,    ///< Displays clock time in seconds.
        SMPTE    ///< Displays industry-standard SMPTE timecode.
    };
    Q_ENUM(TimeCode)

    /**
     * @brief Constructs a TimelineWidget.
     * @param parent The parent widget.
     */
    TimelineWidget(QWidget* parent = nullptr);

    /**
     * @brief Destroys the widget.
     */
    virtual ~TimelineWidget();

    /**
     * @brief Returns the recommended size for the timeline widget.
     * @return The QSize hint for layout management.
     */
    QSize sizeHint() const override;

    /**
     * @brief Returns the current active time range (In/Out points).
     * @return The underlying av::TimeRange.
     */
    av::TimeRange range() const;

    /**
     * @brief Returns the current playhead position.
     * @return The underlying av::Time position.
     */
    av::Time time() const;

    /**
     * @brief Returns whether the timeline is currently tracking user interaction.
     * @return True if the user is actively scrubbing.
     */
    bool tracking() const;

    /**
     * @brief Returns the current timecode display format.
     */
    TimeCode timeCode() const;

public Q_SLOTS:
    /**
     * @brief Sets the active time range for the timeline.
     * @param range The new time range.
     */
    void setRange(const av::TimeRange& range);

    /**
     * @brief Sets the current playhead position.
     * @param time The new time position.
     */
    void setTime(const av::Time& time);

    /**
     * @brief Sets whether the widget emits signals during active scrubbing.
     * @param tracking Enable or disable tracking.
     */
    void setTracking(bool tracking);

    /**
     * @brief Changes the display format of the timeline.
     * @param timecode The format to use (Frames, Time, or SMPTE).
     */
    void setTimeCode(TimelineWidget::TimeCode timeCode);

Q_SIGNALS:
    /**
     * @brief Emitted when the playhead position changes.
     */
    void timeChanged(const av::Time& time);

    /**
     * @brief Emitted when the user is actively moving the timeline slider.
     */
    void sliderMoved(const av::Time& time);

    /**
     * @brief Emitted when the user begins interacting with the timeline.
     */
    void sliderPressed();

    /**
     * @brief Emitted when the user finishes interacting with the timeline.
     */
    void sliderReleased();

protected:
    /**
     * @brief Handles the rendering of the timeline tracks and playhead.
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Handles mouse press events for initiating scrubbing.
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Handles mouse movement for playhead scrubbing.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    /**
     * @brief Handles mouse release events to end interaction.
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    Q_DISABLE_COPY_MOVE(TimelineWidget)
    QScopedPointer<TimelineWidgetPrivate> p;
};

}  // namespace flipman::sdk::widgets

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::widgets::TimelineWidget*)
