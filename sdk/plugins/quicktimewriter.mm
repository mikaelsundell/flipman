// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <plugins/quicktimewriter.h>
#include <QDebug>

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
QuicktimeWriter::is_open() const
{
    return true;
}

bool
QuicktimeWriter::supports_image() const
{
    return false;
}

bool
QuicktimeWriter::supports_audio() const
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
QuicktimeWriter::timerange() const
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
QuicktimeWriter::set_fps(const av::Fps& fps)
{
}

void
QuicktimeWriter::set_timerange(const av::TimeRange& timerange)
{
    return true;
}

bool
QuicktimeWriter::set_metadata(const core::Parameters& metadata)
{
    return true;
}

core::Plugin*
QuicktimeWriter::creator()
{
    return new QuicktimeWriter;
}
