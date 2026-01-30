// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/clip.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/flipmansdk.h>

#include <QColor>
#include <QList>
#include <QScopedPointer>

namespace flipman::sdk::av {

class TrackPrivate;

/**
 * @class Track
 * @brief Represents a single horizontal layer in a Timeline that holds media Clips.
 *
 * Tracks organize Clips into logical groups (e.g., "Video 1", "Audio 1"). The Track
 * manages the relative positioning of Clips and provides the lookup mechanism for
 * determining which media is active at a specific point in time.
 */
class FLIPMANSDK_EXPORT Track : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    /**
     * @brief Constructs a new Track.
     * @param parent The ownership parent for the QObject tree.
     */
    explicit Track(QObject* parent = nullptr);

    /**
     * @brief Destroys the Track and its managed clips.
     * @note Required for the PIMPL pattern to safely delete TrackPrivate.
     */
    ~Track() override;

    /** @name Attributes */
    ///@{
    /**
     * @brief Returns the display name of the track.
     */
    QString name() const;

    /**
     * @brief Returns the color used to represent the track in a UI.
     */
    QColor color() const;
    ///@}



    /** @name Clip Management */
    ///@{
    /**
     * @brief Returns the TimeRange that a specific clip occupies on this track.
     * @param clip The clip to query.
     */
    TimeRange clipRange(Clip* clip) const;

    /**
     * @brief Returns a list of all clips currently managed by this track.
     */
    QList<Clip*> clips() const;

    /**
     * @brief Checks if a specific clip is a member of this track.
     * @param clip The clip to look for.
     */
    bool containsClip(Clip* clip) const;
    ///@}

    /** @name Status */
    ///@{
    /**
     * @brief Returns the current error state of the track.
     */
    core::Error error() const;

    /**
     * @brief Resets the track, removing all clips and clearing internal state.
     */
    void reset();
    ///@}

public Q_SLOTS:
    /** @name Setters */
    ///@{
    /**
     * @brief Sets the display name of the track.
     */
    void setName(const QString& name);

    /**
     * @brief Sets the display color of the track.
     */
    void setColor(const QColor& color);

    /**
     * @brief Adds a clip to the track at a specific time range.
     * @param clip The clip to insert.
     * @param range The start and duration of the clip on the timeline.
     */
    void insertClip(Clip* clip, const TimeRange& range);

    /**
     * @brief Removes a clip from the track.
     */
    void removeClip(Clip* clip);
    ///@}

Q_SIGNALS:
    /** @name Notifications */
    ///@{
    /**
     * @brief Emitted when the track's name is modified.
     */
    void nameChanged(const QString& name);

    /**
     * @brief Emitted when the track's display color is modified.
     */
    void colorChanged(const QColor& color);
    ///@}

private:
    Q_DISABLE_COPY_MOVE(Track)
    QScopedPointer<TrackPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Track*)
