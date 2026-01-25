// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QDir>
#include <QImage>
#include <QImageWriter>
#include <QPointer>
#include <core/os.h>
#include <plugins/qtwriter.h>

class QtWriterPrivate : public QSharedData {
public:
    QtWriterPrivate();
    static plugins::PluginHandler::Info info();
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

QtWriterPrivate::QtWriterPrivate() { d.fps = av::Fps::fps_24(); }

plugins::PluginHandler::Info
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
QtWriter::is_open() const
{
    return true;
}

bool
QtWriter::supports_image() const
{
    return true;
}

bool
QtWriter::supports_audio() const
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
    QString filename = p->d.file.filename(frame);
    core::ImageBuffer copy = image;
    if (copy.imageformat() != core::ImageFormat::UINT8 || copy.channels() != 4) {
        copy = core::ImageBuffer::convert(copy, core::ImageFormat::UINT8, 4);
    }
    QImage copyimage(reinterpret_cast<quint8*>(image.data()), copy.datawindow().width(), copy.datawindow().height(),
                     copy.stridesize(), QImage::Format_ARGB32);
    p->d.timestamp.set_ticks(p->d.timestamp.ticks() + p->d.timestamp.tpf());
    p->d.error = core::Error();
    if (!copyimage.save(filename)) {
        p->d.error = core::Error(p->info().name, QString("could not save to filename: %1").arg(filename));
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
QtWriter::timerange() const
{
    return p->d.timerange;
}

core::Error
QtWriter::error() const
{
    return p->d.error;
}

void
QtWriter::set_fps(const av::Fps& fps)
{
    p->d.fps = fps;
}

void
QtWriter::set_timerange(const av::TimeRange& timerange)
{
    p.detach();
    seek(timerange);
}

bool
QtWriter::set_metadata(const core::Parameters& metadata)
{
    p.detach();
    p->d.metadata = metadata;
}

plugins::PluginHandler
QtWriter::handler()
{
    static plugins::PluginHandler handler = plugins::PluginHandler::create<MediaWriter>(QtWriterPrivate::info(),
                                                                                        QtWriterPrivate::extensions,
                                                                                        QtWriterPrivate::creator);
    return handler;
}
