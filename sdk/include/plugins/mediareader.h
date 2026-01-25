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
class MediaReaderPrivate;
class MediaReader : public core::Plugin {
public:
    MediaReader(QObject* parent = nullptr);
    virtual ~MediaReader();
    virtual bool open(const core::File& file, core::Parameters parameters = core::Parameters()) = 0;
    virtual bool close() = 0;
    virtual bool is_open() const = 0;
    virtual bool supports_image() const = 0;
    virtual bool supports_audio() const = 0;
    virtual QList<QString> extensions() const = 0;
    virtual av::Time read() = 0;
    virtual av::Time skip() = 0;
    virtual av::Time seek(const av::TimeRange& range) = 0;
    virtual av::Time start() const = 0;
    virtual av::Time time() const = 0;
    virtual av::Fps fps() const = 0;
    virtual av::TimeRange timerange() const = 0;
    virtual core::AudioBuffer audio() const;
    virtual core::ImageBuffer image() const;
    virtual core::Parameters parameters() const;
    virtual core::Parameters metadata() const;
};
}  // namespace plugins
