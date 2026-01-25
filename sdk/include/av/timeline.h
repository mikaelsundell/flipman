// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/fps.h>
#include <av/renderlayer.h>
#include <av/smptetime.h>
#include <av/time.h>
#include <av/timerange.h>
#include <av/track.h>
#include <core/container.h>

#include <QImage>
#include <QObject>
#include <QScopedPointer>

namespace av {
class TimelinePrivate;
class Timeline : public core::Container {
    Q_OBJECT
public:
    Timeline(QObject* parent = nullptr);
    virtual ~Timeline();
    bool is_playing() const;
    TimeRange timerange() const;
    TimeRange io() const;
    Time time() const;
    Time starttime() const;
    SmpteTime timecode() const;
    Fps fps() const;
    int width() const;
    int height() const;
    bool loop() const;
    bool has_track(Track* track);
    QList<Track*> tracks() const;
    int threadcount() const;
    core::Error error() const;
    void reset();

public Q_SLOTS:
    void insert_track(Track* track);
    void remove_track(Track* track);
    void set_timerange(const TimeRange& timerange);
    void set_io(const TimeRange& io);
    void set_everyframe(bool everyframe);
    void set_width(int width);
    void set_height(int height);
    void set_loop(bool loop);
    void set_threadcount(int threadcount);
    void seek(const Time& time);
    void play();
    void stop();

Q_SIGNALS:
    void timerange_changed(const TimeRange& timerange);
    void io_changed(const TimeRange& io);
    void time_changed(const Time& time);
    void timecode_changed(const Time& time);
    void renderlayer_changed(const RenderLayer& renderlayer);
    void audio_changed(const QByteArray& buffer);
    void loop_changed(bool loop);
    void everyframe_changed(bool everyframe);
    void actualfps_changed(qreal fps);
    void width_changed(int width);
    void height_changed(int height);
    void play_changed(bool streaming);

private:
    QScopedPointer<TimelinePrivate> p;
};
}  // namespace av
