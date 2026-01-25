// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/fps.h>
#include <av/time.h>
#include <av/timerange.h>
#include <core/audiobuffer.h>
#include <core/error.h>
#include <core/file.h>
#include <core/imagebuffer.h>
#include <core/parameters.h>
#include <core/plugin.h>

#include <QExplicitlySharedDataPointer>

namespace plugins {
class MediaWriterPrivate;
class MediaWriter : public core::Plugin {
public:
    MediaWriter(QObject* parent = nullptr);
    virtual ~MediaWriter();
    virtual bool open(const core::File& file, core::Parameters parameters = core::Parameters()) = 0;
    virtual bool close() = 0;
    virtual bool is_open() const = 0;
    virtual bool supports_image() const = 0;
    virtual bool supports_audio() const = 0;
    virtual QList<QString> extensions() const = 0;
    virtual av::Time write(const core::AudioBuffer& image);
    virtual av::Time write(const core::ImageBuffer& image);
    virtual av::Time seek(const av::TimeRange& range);
    virtual av::Time time() const;
    virtual av::Fps fps() const;
    virtual av::TimeRange timerange() const;

    virtual void set_fps(const av::Fps& fps);
    virtual void set_timerange(const av::TimeRange& timerange);
    virtual bool set_metadata(const core::Parameters& metadata);
};
}  // namespace plugins
