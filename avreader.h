// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include "avfps.h"
#include "avmetadata.h"
#include "avsidecar.h"
#include "avsmptetime.h"
#include "avtime.h"
#include "avtimerange.h"

#include <QImage>
#include <QObject>
#include <QScopedPointer>

class AVReaderPrivate;
class AVReader : public QObject {
    Q_OBJECT
    public:
        enum Error { NO_ERROR, FILE_ERROR, API_ERROR, OTHER_ERROR };
        Q_ENUM(Error)

    public:
        AVReader();
        virtual ~AVReader();
        void open(const QString& filename);
        void read();
        void close();
        bool is_open() const;
        bool is_closed() const;
        bool is_streaming() const;
        bool is_supported(const QString& extension) const;
        QString filename() const;
        QString title() const;
        AVTimeRange range() const;
        AVTime time() const;
        AVFps fps() const;
        bool loop() const;
        AVSmpteTime smptetime() const;
        AVSmpteTime start() const;
        AVMetadata metadata();
        AVSidecar sidecar();
        QList<QString> extensions() const;
        AVReader::Error error() const;
        QString error_message() const;

    public Q_SLOTS:
        void set_loop(bool loop);
        void set_everyframe(bool everyframe);
        void seek(const AVTime& time);
        void stream();
        void stop();

    Q_SIGNALS:
        void opened(const QString& filename);
        void time_changed(const AVTime& time);
        void smptetime_changed(const AVSmpteTime& timecode);
        void range_changed(const AVTimeRange& timerange);
        void video_changed(const QImage& image);
        void audio_changed(const QByteArray& buffer);
        void loop_changed(bool loop);
        void everyframe_changed(bool everyframe);
        void stream_changed(bool streaming);
        void actual_fps_changed(qreal fps);

    private:
        QScopedPointer<AVReaderPrivate> p;
};
