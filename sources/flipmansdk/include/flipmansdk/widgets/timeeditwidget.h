// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/time.h>
#include <flipmansdk/flipmansdk.h>

#include <QScopedPointer>
#include <QWidget>

namespace flipman::sdk::widgets {

class TimeEditWidgetPrivate;

/**
 * @class TimeEditWidget
 * @brief A specialized input widget for manipulating and displaying temporal data.
 *
 * TimeEditWidget provides a user interface for editing av::Time values using
 * multiple professional formats including raw frames, absolute time, and
 * SMPTE timecode (HH:MM:SS:FF). It is designed to integrate seamlessly into
 * playbacks and timeline controls within the Flipman SDK.
 */
class FLIPMANSDK_EXPORT TimeEditWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(TimeCode timeCode READ timeCode WRITE setTimeCode)

public:
    /**
     * @enum TimeCode
     * @brief Defines the display and input format for the time value.
     */
    enum TimeCode {
        Frames,  ///< Displays the total number of frames (e.g., "101").
        Time,    ///< Displays the absolute time in seconds (e.g., "00:04.20").
        SMPTE    ///< Displays standard SMPTE timecode (e.g., "00:00:04:05").
    };
    Q_ENUM(TimeCode)

    /**
     * @brief Constructs a TimeEditWidget.
     * @param parent The parent widget.
     */
    explicit TimeEditWidget(QWidget* parent = nullptr);

    /**
     * @brief Destroys the widget.
     * @note Required for the PIMPL pattern to safely delete TimeEditWidgetPrivate.
     */
    ~TimeEditWidget() override;

    /** @name Attributes */
    ///@{
    /**
     * @brief Returns the current time value.
     * @return The underlying av::Time object containing frame rate and position.
     */
    av::Time time() const;

    /**
     * @brief Returns the current display format.
     */
    TimeCode timeCode() const;
    ///@}



public Q_SLOTS:
    /** @name Setters */
    ///@{
    /**
     * @brief Sets the current time value to be displayed and edited.
     * @param time The new time value.
     */
    void setTime(const av::Time& time);

    /**
     * @brief Changes the current timecode display format.
     * @param timeCode The format to use (Frames, Time, or SMPTE).
     */
    void setTimeCode(TimeEditWidget::TimeCode timeCode);
    ///@}

protected:
    /**
     * @brief Handles custom rendering of the time display.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    Q_DISABLE_COPY_MOVE(TimeEditWidget)
    QScopedPointer<TimeEditWidgetPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::widgets

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::widgets::TimeEditWidget*)
