// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/av/track.h>

#include <flipmansdk/render/imagelayer.h>

#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::av {

class TimelinePrivate;

/**
 * @class Timeline
 * @brief Multi-track playback and composition controller.
 */
class FLIPMANSDK_EXPORT Timeline : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

public:
    /**
     * @brief Constructs a Timeline.
     */
    explicit Timeline(QObject* parent = nullptr);

    /**
     * @brief Destroys the Timeline.
     */
    ~Timeline() override;

    /** @name State */
    ///@{

    /**
     * @brief Returns true if playing.
     */
    bool isPlaying() const;

    /**
     * @brief Returns true if loop enabled.
     */
    bool loop() const;

    /**
     * @brief Returns the width.
     */
    int width() const;

    /**
     * @brief Returns the height.
     */
    int height() const;

    ///@}

    /** @name Temporal */
    ///@{

    /**
     * @brief Returns the time range.
     */
    TimeRange timeRange() const;

    /**
     * @brief Returns the I/O range.
     */
    TimeRange ioRange() const;

    /**
     * @brief Returns the current time.
     */
    Time time() const;

    /**
     * @brief Returns the start time.
     */
    Time startTime() const;

    /**
     * @brief Returns the timecode.
     */
    SmpteTime timeCode() const;

    /**
     * @brief Returns the frame rate.
     */
    Fps fps() const;

    ///@}

    /** @name Tracks */
    ///@{

    /**
     * @brief Returns true if track exists.
     */
    bool hasTrack(Track* track) const;

    /**
     * @brief Returns all tracks.
     */
    QList<Track*> tracks() const;

    ///@}

    /** @name Configuration */
    ///@{

    /**
     * @brief Returns the thread count.
     */
    int threadCount() const;

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
    /** @name Composition */
    ///@{

    /**
     * @brief Inserts a track.
     */
    void insertTrack(Track* track);

    /**
     * @brief Removes a track.
     */
    void removeTrack(Track* track);

    /**
     * @brief Sets the time range.
     */
    void setTimeRange(const TimeRange& timeRange);

    /**
     * @brief Sets the I/O range.
     */
    void setIoRange(const TimeRange& ioRange);

    /**
     * @brief Sets the width.
     */
    void setWidth(int width);

    /**
     * @brief Sets the height.
     */
    void setHeight(int height);

    ///@}

    /** @name Playback */
    ///@{

    /**
     * @brief Enables or disables every-frame mode.
     */
    void setEveryFrame(bool everyFrame);

    /**
     * @brief Enables or disables loop.
     */
    void setLoop(bool loop);

    /**
     * @brief Sets the thread count.
     */
    void setThreadCount(int threadCount);

    /**
     * @brief Seeks to time.
     */
    void seek(const Time& time);

    /**
     * @brief Starts playback.
     */
    void play();

    /**
     * @brief Stops playback.
     */
    void stop();

    ///@}

Q_SIGNALS:
    /** @name Notifications */
    ///@{

    void timerangeChanged(const TimeRange& timeRange);
    void ioRangeChanged(const TimeRange& io);
    void timeChanged(const Time& time);
    void timeCodeChanged(const Time& time);
    void loopChanged(bool loop);
    void everyFrameChanged(bool everyFrame);
    void actualFpsChanged(qreal fps);
    void widthChanged(int width);
    void heightChanged(int height);
    void playChanged(bool playing);

    ///@}

    /** @name Output */
    ///@{

    /**
     * @brief Emitted when image layer changes.
     */
    void imageLayerChanged(const render::ImageLayer& imageLayer);

    /**
     * @brief Emitted when audio changes.
     */
    void audioChanged(const QByteArray& buffer);

    ///@}

private:
    Q_DISABLE_COPY_MOVE(Timeline)
    QScopedPointer<TimelinePrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Timeline*)
