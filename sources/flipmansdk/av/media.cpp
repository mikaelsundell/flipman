// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/av/media.h>

#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/pluginregistry.h>

#include <QEventLoop>
#include <QFileInfo>
#include <QPointer>
#include <QTimer>

namespace flipman::sdk::av {
class MediaPrivate : public QSharedData {
public:
    plugins::MediaReader* reader(const QString& extension);
    struct Data {
        bool open;
        core::File file;
        core::Error error;
        QScopedPointer<plugins::MediaReader> reader;
    };
    Data d;
};

plugins::MediaReader*
MediaPrivate::reader(const QString& extension)
{
    auto* registry = plugins::PluginRegistry::instance();
    plugins::MediaReader* reader = registry->getPlugin<plugins::MediaReader>(extension);
    if (!reader) {
        d.error = core::Error("Media", QStringLiteral("No MediaReader registered for extension: %1").arg(extension));
        return nullptr;
    }
    d.reader.reset(reader);  // MediaPrivate now OWNS the reader
    return d.reader.get();
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
    p->d.file = file;
    p->d.open = false;
    p->d.error.reset();

    plugins::MediaReader* reader = p->reader(file.extension());

    if (!reader) {
        return false;
    }
    connect(reader, &plugins::MediaReader::opened, this, [this]() {
        p->d.open = true;
        Q_EMIT opened();
    });
    const bool started = reader->open(file);
    if (started && reader->isOpen()) {
        p->d.open = true;
        Q_EMIT opened();
    }
    if (!started) {
        p->d.error = reader->error();
    }
    return started;
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
    return registry->hasExtension<plugins::MediaReader>(extension);
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

bool
Media::waitForOpened(int msecs)
{
    if (p->d.open)
        return true;
    QEventLoop loop;
    QTimer timer;
    QObject::connect(this, &Media::opened, &loop, &QEventLoop::quit);
    if (msecs >= 0) {
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(msecs);
    }
    if (p->d.open)
        return true;
    loop.exec();
    return p->d.open;
}

}  // namespace flipman::sdk::av
