// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/clip.h>
#include <flipmansdk/av/timerange.h>

#include <QColor>
#include <QList>
#include <QScopedPointer>

namespace flipman::sdk::av {

class TrackPrivate;

/**
 * @class Track
 * @brief Timeline track containing Clips.
 */
class FLIPMANSDK_EXPORT Track : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    /**
     * @brief Constructs a Track.
     */
    explicit Track(QObject* parent = nullptr);

    /**
     * @brief Destroys the Track.
     */
    ~Track() override;

    /** @name Attributes */
    ///@{

    /**
     * @brief Returns the name.
     */
    QString name() const;

    /**
     * @brief Returns the color.
     */
    QColor color() const;

    ///@}

    /** @name Clips */
    ///@{

    /**
     * @brief Returns the clip range.
     */
    TimeRange clipRange(Clip* clip) const;

    /**
     * @brief Returns all clips.
     */
    QList<Clip*> clips() const;

    /**
     * @brief Returns true if clip exists.
     */
    bool containsClip(Clip* clip) const;

    ///@}

    /** @name Status */
    ///@{

    /**
     * @brief Returns the error state.
     */
    core::Error error() const;

    /**
     * @brief Resets to default state.
     */
    void reset();

    ///@}

public Q_SLOTS:
    /** @name Setters */
    ///@{

    /**
     * @brief Sets the name.
     */
    void setName(const QString& name);

    /**
     * @brief Sets the color.
     */
    void setColor(const QColor& color);

    /**
     * @brief Inserts a clip.
     */
    void insertClip(Clip* clip, const TimeRange& range);

    /**
     * @brief Removes a clip.
     */
    void removeClip(Clip* clip);

    ///@}

Q_SIGNALS:
    /** @name Notifications */
    ///@{

    /**
     * @brief Emitted when name changes.
     */
    void nameChanged(const QString& name);

    /**
     * @brief Emitted when color changes.
     */
    void colorChanged(const QColor& color);

    ///@}

private:
    Q_DISABLE_COPY_MOVE(Track)
    QScopedPointer<TrackPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Track*)
