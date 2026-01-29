// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QFileInfo>
#include <QPointer>
#include <flipmansdk/av/media.h>
#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/pluginregistry.h>

namespace flipman::sdk::av {
class MediaPrivate : public QSharedData {
public:
    bool open(const core::File& file);
    struct Data {
        core::File file;
        core::Error error;
        QScopedPointer<plugins::MediaReader> reader;
    };
    Data d;
};


bool
MediaPrivate::open(const core::File& file)
{
    d.file = file;
    plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
    d.reader.reset(registry->get_plugin<plugins::MediaReader>(file.extension()));
    if (d.reader) {
        if (d.reader->open(file)) {
            return true;
        }
    }
    d.error = d.reader->error();
    return false;
}

Media::Media()
    : p(new MediaPrivate())
{}

Media::Media(const Media& other)
    : p(other.p)
{}

Media::~Media() {}

bool
Media::open(const core::File& file)
{
    return p->open(file);
}

bool
Media::close()
{
    return p->d.reader->close();
}

bool
Media::isOpen() const
{
    p->d.reader->isOpen();
    return true;
}

bool
Media::isSupported(const QString& extension) const
{
    plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
    return registry->has_extension<plugins::MediaReader>(extension);
}

bool
Media::isValid() const
{
    return p->d.reader ? true : false;
}

Time
Media::read()
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->read();
}

Time
Media::seek(const TimeRange& range) const
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->seek(range);
}

Time
Media::start() const
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->start();
}

Time
Media::time() const
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->time();
}

Fps
Media::fps() const
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->fps();
}

TimeRange
Media::timeRange() const
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->timeRange();
}

core::File
Media::file() const
{
    return p->d.file;
}

core::AudioBuffer
Media::audio() const
{
    Q_ASSERT("media is not open and must support audio" && isOpen() && p->d.reader->supportsAudio());
    return p->d.reader->audio();
}

core::ImageBuffer
Media::image() const
{
    Q_ASSERT("media is not open and must support images" && isOpen() && p->d.reader->supportsImage());
    return p->d.reader->image();
}

core::Parameters
Media::parameters() const
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->parameters();
}

core::Parameters
Media::metaData() const
{
    Q_ASSERT("media is not open" && isOpen());
    return p->d.reader->metaData();
}

core::Error
Media::error() const
{
    return p->d.reader->error();
}

void
Media::reset()
{
    p.reset(new MediaPrivate());
}

Media&
Media::operator=(const Media& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
Media::operator==(const Media& other) const
{
    return this->p == other.p;
}

bool
Media::operator!=(const Media& other) const
{
    return !(this->p == other.p);
}
}  // namespace flipman::sdk::av
