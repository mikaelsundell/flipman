// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QPointer>
#include <plugins/mediawriter.h>

namespace plugins {
MediaWriter::MediaWriter(QObject* parent)
    : core::Plugin(parent)
{}

MediaWriter::~MediaWriter() {}

av::Time
MediaWriter::write(const core::AudioBuffer& image)
{
    return av::Time();
}

av::Time
MediaWriter::write(const core::ImageBuffer& image)
{
    return av::Time();
}

av::Time
MediaWriter::seek(const av::TimeRange& range)
{
    return av::Time();
}

av::Time
MediaWriter::time() const
{
    return av::Time();
}

av::Fps
MediaWriter::fps() const
{
    return av::Fps();
}

av::TimeRange
MediaWriter::timerange() const
{
    return av::TimeRange();
}

void
MediaWriter::set_fps(const av::Fps& fps)
{}

void
MediaWriter::set_timerange(const av::TimeRange& timerange)
{}

bool
MediaWriter::set_metadata(const core::Parameters& metadata)
{}
}  // namespace plugins
