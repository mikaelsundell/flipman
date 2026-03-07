// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/time.h>
#include <flipmansdk/flipmansdk.h>

#include <QScopedPointer>
#include <QWidget>

namespace flipman::sdk::widgets {

class TimeEditPrivate;

/**
 * @class TimeEdit
 * @brief Widget for editing and displaying av::Time values.
 *
 * Supports multiple display formats including frames, time, and SMPTE.
 */
class FLIPMANSDK_EXPORT TimeEdit : public QWidget {
    Q_OBJECT
    Q_PROPERTY(TimeCode timeCode READ timeCode WRITE setTimeCode)

public:
    /**
     * @enum TimeCode
     * @brief Display format for the time value.
     */
    enum TimeCode {
        Frames,  ///< Displays total frame count.
        Time,    ///< Displays absolute time.
        SMPTE    ///< Displays SMPTE timecode.
    };
    Q_ENUM(TimeCode)

public:
    /**
     * @brief Constructs a TimeEdit.
     *
     * @param parent Optional parent widget.
     */
    explicit TimeEdit(QWidget* parent = nullptr);

    /**
     * @brief Destroys the TimeEdit.
     */
    ~TimeEdit() override;

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the current time value.
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
     * @brief Sets the current time value.
     */
    void setTime(const av::Time& time);

    /**
     * @brief Sets the display format.
     */
    void setTimeCode(TimeEdit::TimeCode timeCode);

    ///@}

protected:
    /**
     * @brief Paint event handler.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    Q_DISABLE_COPY_MOVE(TimeEdit)
    QScopedPointer<TimeEditPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::widgets

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::widgets::TimeEdit*)
