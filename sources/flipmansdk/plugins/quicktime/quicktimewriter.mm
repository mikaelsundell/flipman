// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <plugins/quicktime/quicktime.h>
#include <QDebug>

namespace flipman::sdk::plugins {

class QuicktimeWriterPrivate : public QSharedData {
public:
    struct Data
    {
    };
    Data d;
};

QuicktimeWriter::QuicktimeWriter(QObject* parent)
: plugins::MediaWriter(parent)
{
}

QuicktimeWriter::~QuicktimeWriter()
{
}

bool
QuicktimeWriter::open(const core::File& file, core::Parameters parameters)
{
    return true;
}

bool
QuicktimeWriter::close()
{
    return true;
}

bool
QuicktimeWriter::isOpen() const
{
    return true;
}

bool
QuicktimeWriter::supportsImage() const
{
    return false;
}

bool
QuicktimeWriter::supportsAudio() const
{
    return false;
}

av::Time
QuicktimeWriter::write(const core::AudioBuffer& image)
{
    return av::Time();
}

av::Time
QuicktimeWriter::write(const core::ImageBuffer& image)
{
    return av::Time();
}

av::Time
QuicktimeWriter::seek(const av::TimeRange& range)
{
    return av::Time();
}

av::Time
QuicktimeWriter::time() const
{
    return av::Time();
}

av::Fps
QuicktimeWriter::fps() const
{
    return av::Fps();
}

av::TimeRange
QuicktimeWriter::timeRange() const
{
    return av::TimeRange();
}

QList<QString>
QuicktimeWriter::extensions() const
{
    return QList<QString>();
}

core::Error
QuicktimeWriter::error() const
{
    return core::Error();
}

void
QuicktimeWriter::setFps(const av::Fps& fps)
{
}

void
QuicktimeWriter::setTimeRange(const av::TimeRange& timerange)
{
    return true;
}

bool
QuicktimeWriter::setMetaData(const core::Parameters& metadata)
{
    return true;
}

core::Plugin*
QuicktimeWriter::creator()
{
    return new QuicktimeWriter;
}

}
