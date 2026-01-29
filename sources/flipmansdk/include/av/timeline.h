// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <av/fps.h>
#include <av/renderlayer.h>
#include <av/smptetime.h>
#include <av/time.h>
#include <av/timerange.h>
#include <av/track.h>

#include <QImage>
#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::av {

class TimelinePrivate;

/**
 * @class Timeline
 * @brief The central coordinator for multi-track media playback and composition.
 *
 * Timeline manages the temporal and spatial organization of media tracks. It handles
 * playback logic (play/stop/seek), loop behavior, and I/O (In/Out) ranges. As the
 * playhead moves, the Timeline synthesizes the state of all active tracks into
 * renderable layers and audio streams.
 */
class FLIPMANSDK_EXPORT Timeline : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

public:
    /**
     * @brief Constructs a new Timeline.
     * @param parent The ownership parent.
     */
    explicit Timeline(QObject* parent = nullptr);

    /**
     * @brief Destroys the Timeline and all managed tracks.
     */
    virtual ~Timeline();

    /** @name State Query */
    ///@{
    bool isPlaying() const;
    bool loop() const;
    int width() const;
    int height() const;
    ///@}

    /** @name Temporal Properties */
    ///@{
    /**
     * @brief Returns the total duration/range of the timeline.
     */
    TimeRange timeRange() const;

    /**
     * @brief Returns the current In/Out (working) range.
     */
    TimeRange ioRange() const;

    /**
     * @brief Returns the current playhead position.
     */
    Time time() const;

    /**
     * @brief Returns the starting time of the timeline (e.g., 01:00:00:00).
     */
    Time startTime() const;

    /**
     * @brief Returns the current playhead position formatted as SMPTE timecode.
     */
    SmpteTime timeCode() const;

    /**
     * @brief Returns the master frame rate of the timeline.
     */
    Fps fps() const;
    ///@}

    /** @name Track Management */
    ///@{
    bool hasTrack(Track* track);
    QList<Track*> tracks() const;
    ///@}

    /** @name Engine Configuration */
    ///@{
    /**
     * @brief Returns the number of threads allocated for background decoding/rendering.
     */
    int threadCount() const;

    core::Error error() const;
    void reset();
    ///@}

public Q_SLOTS:
    /** @name Composition Control */
    ///@{
    void insertTrack(Track* track);
    void removeTrack(Track* track);
    void setTimeRange(const TimeRange& timeRange);
    void setIoRange(const TimeRange& ioRange);
    void setWidth(int width);
    void setHeight(int height);
    ///@}

    /** @name Playback Control */
    ///@{
    void setEveryFrame(bool everyFrame);
    void setLoop(bool loop);
    void setThreadCount(int threadCount);

    /**
     * @brief Moves the playhead to a specific time.
     */
    void seek(const Time& time);

    /**
     * @brief Starts playback.
     */
    void play();

    /**
     * @brief Pauses playback.
     */
    void stop();
    ///@}

Q_SIGNALS:
    /** @name Property Notifications */
    ///@{
    void timerangeChanged(const TimeRange& timeRange);
    void ioRangeChanged(const TimeRange& io);
    void timeChanged(const Time& time);
    void timeCodeChanged(const Time& time);
    void loopChanged(bool loop);
    void everyFrameChanged(bool everyframe);
    void actualFpsChanged(qreal fps);
    void widthChanged(int width);
    void heightChanged(int height);
    void playChanged(bool playing);
    ///@}

    /** @name Data Production Signals
     * These signals deliver the results of the timeline composition.
     */
    ///@{
    /**
     * @brief Emitted when a new frame is ready for display.
     */
    void renderLayerChanged(const RenderLayer& renderLayer);

    /**
     * @brief Emitted when a new audio buffer is ready for playback.
     */
    void audioChanged(const QByteArray& buffer);
    ///@}

private:
    Q_DISABLE_COPY_MOVE(Timeline)
    QScopedPointer<TimelinePrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Timeline*)
