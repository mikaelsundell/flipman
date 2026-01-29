// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/plugins/qt/qt.h>

#include <QDir>
#include <QImage>
#include <QImageWriter>
#include <QPointer>

namespace flipman::sdk::plugins {

class QtWriterPrivate : public QSharedData {
public:
    QtWriterPrivate();
    static PluginHandler::Info info();
    static core::Plugin* creator();
    static QList<QString> extensions();
    struct Data {
        core::File file;
        av::Fps fps;
        av::TimeRange timerange;
        av::Time timestamp;
        core::Parameters parameters;
        core::Parameters metadata;
        core::Error error;
    };
    Data d;
};

QtWriterPrivate::QtWriterPrivate() { d.fps = av::Fps::fps24(); }

PluginHandler::Info
QtWriterPrivate::info()
{
    return { "qtwriter", "writes multimedia files using Qt's built-in classes", "1.0.0" };
}

core::Plugin*
QtWriterPrivate::creator()
{
    return new QtWriter();
}

QList<QString>
QtWriterPrivate::extensions()
{
    static QList<QString> extensions = []() {
        QList<QString> formats;
        for (const QByteArray& format : QImageWriter::supportedImageFormats()) {
            formats.append(QString(format).toLower());
        }
        return formats;
    }();
    return extensions;
};

QtWriter::QtWriter(QObject* object)
    : plugins::MediaWriter(object)
    , p(new QtWriterPrivate())
{}

QtWriter::~QtWriter() {}

bool
QtWriter::open(const core::File& file, core::Parameters parameters)
{
    p->d.file = file;
    p->d.parameters = parameters;
}

bool
QtWriter::close()
{}

bool
QtWriter::isOpen() const
{
    return true;
}

bool
QtWriter::supportsImage() const
{
    return true;
}

bool
QtWriter::supportsAudio() const
{
    return false;
}

QList<QString>
QtWriter::extensions() const
{
    return p->extensions();
}

av::Time
QtWriter::write(const core::ImageBuffer& image)
{
    qint64 frame = p->d.timestamp.frames();
    QString fileName = p->d.file.fileName(frame);
    core::ImageBuffer copy = image;
    if (copy.imageFormat() != core::ImageFormat::UINT8 || copy.channels() != 4) {
        copy = core::ImageBuffer::convert(copy, core::ImageFormat::UINT8, 4);
    }
    QImage copyImage(reinterpret_cast<quint8*>(image.data()), copy.dataWindow().width(), copy.dataWindow().height(),
                     copy.strideSize(), QImage::Format_ARGB32);
    p->d.timestamp.setTicks(p->d.timestamp.ticks() + p->d.timestamp.tpf());
    p->d.error = core::Error();
    if (!copyImage.save(fileName)) {
        p->d.error = core::Error(p->info().name, QString("could not save to filename: %1").arg(fileName));
    }
    return p->d.timestamp;
}

av::Time
QtWriter::seek(const av::TimeRange& timerange)
{
    p.detach();
    p->d.timerange = timerange;
    p->d.timestamp = p->d.timerange.start();
    return p->d.timestamp;
}

av::Time
QtWriter::time() const
{
    return p->d.timestamp;
}

av::Fps
QtWriter::fps() const
{
    return p->d.fps;
}

av::TimeRange
QtWriter::timeRange() const
{
    return p->d.timerange;
}

core::Error
QtWriter::error() const
{
    return p->d.error;
}

void
QtWriter::setFps(const av::Fps& fps)
{
    p->d.fps = fps;
}

void
QtWriter::setTimeRange(const av::TimeRange& timeRange)
{
    p.detach();
    seek(timeRange);
}

bool
QtWriter::setMetaData(const core::Parameters& metaData)
{
    p.detach();
    p->d.metadata = metaData;
}

plugins::PluginHandler
QtWriter::handler()
{
    static plugins::PluginHandler handler = plugins::PluginHandler::create<MediaWriter>(QtWriterPrivate::info(),
                                                                                        QtWriterPrivate::extensions,
                                                                                        QtWriterPrivate::creator);
    return handler;
}

}  // namespace flipman::sdk::plugins
