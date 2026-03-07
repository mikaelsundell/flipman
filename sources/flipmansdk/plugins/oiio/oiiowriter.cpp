// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/plugins/oiio/oiiowriter.h>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/typedesc.h>

using namespace OIIO;

namespace flipman::sdk::plugins {

class OIIOWriterPrivate : public QSharedData {
public:
    OIIOWriterPrivate();
    av::Time write(const core::ImageBuffer& image);
    OIIO::TypeDesc toTypeDesc(core::ImageFormat::Type imageFormat);
    static PluginHandler::Info info();
    static core::Plugin* creator();
    static QList<QString> extensions();
    struct Data {
        core::File file;
        av::Fps fps;
        av::TimeRange timerange;
        av::Time timestamp;
        core::MetaData metaData;
        core::Error error;
    };
    Data d;
};

OIIOWriterPrivate::OIIOWriterPrivate() { d.fps = av::Fps::fps24(); }

av::Time
OIIOWriterPrivate::write(const core::ImageBuffer& image)
{
    const qint64 frame = d.timestamp.frames();
    QString fileName = d.file.fileName(frame);

    const int width = image.dataWindow().width();
    const int height = image.dataWindow().height();
    const int channels = image.channels();

    const OIIO::TypeDesc type = toTypeDesc(image.imageFormat().type());

    if (type == OIIO::TypeDesc::UNKNOWN) {
        d.error = core::Error("oiiowriter", "unsupported pixel format");
        return d.timestamp;
    }

    OIIO::ImageSpec spec(width, height, channels, type);

    auto out = OIIO::ImageOutput::create(fileName.toStdString());

    if (!out) {
        d.error = core::Error("oiiowriter", "could not create image output");
        return d.timestamp;
    }

    if (!out->open(fileName.toStdString(), spec)) {
        d.error = core::Error("oiiowriter", out->geterror().c_str());
        return d.timestamp;
    }

    const bool ok = out->write_image(type, image.data(), image.pixelSize(), image.strideSize());

    out->close();

    if (!ok) {
        d.error = core::Error("oiiowriter", out->geterror().c_str());
        return d.timestamp;
    }

    d.timestamp.setTicks(d.timestamp.ticks() + d.timestamp.tpf());

    return d.timestamp;
}

OIIO::TypeDesc
OIIOWriterPrivate::toTypeDesc(core::ImageFormat::Type t)
{
    using T = core::ImageFormat::Type;
    switch (t) {
    case T::UInt8: return OIIO::TypeDesc::UINT8;
    case T::Int8: return OIIO::TypeDesc::INT8;

    case T::UInt16: return OIIO::TypeDesc::UINT16;
    case T::Int16: return OIIO::TypeDesc::INT16;

    case T::UInt32: return OIIO::TypeDesc::UINT32;
    case T::Int32: return OIIO::TypeDesc::INT32;

    case T::UInt64: return OIIO::TypeDesc::UINT64;
    case T::Int64: return OIIO::TypeDesc::INT64;

    case T::Half: return OIIO::TypeDesc::HALF;
    case T::Float: return OIIO::TypeDesc::FLOAT;
    case T::Double: return OIIO::TypeDesc::DOUBLE;
    default: return OIIO::TypeDesc::UNKNOWN;
    }
}

PluginHandler::Info
OIIOWriterPrivate::info()
{
    return { "oiiowriter", "writes images using OpenImageIO", "1.0.0" };
}

core::Plugin*
OIIOWriterPrivate::creator()
{
    return new OIIOWriter();
}

QList<QString>
OIIOWriterPrivate::extensions()
{
    static QList<QString> extensions = []() {
        QList<QString> formats;
        std::string list = OIIO::get_string_attribute("extension_list");
        std::stringstream formatsStream(list);
        std::string formatEntry;

        while (std::getline(formatsStream, formatEntry, ';')) {
            auto colon = formatEntry.find(':');
            if (colon == std::string::npos)
                continue;

            std::string extList = formatEntry.substr(colon + 1);
            std::stringstream extStream(extList);
            std::string ext;
            while (std::getline(extStream, ext, ',')) {
                QString qext = QString::fromStdString(ext).toLower();
                if (!formats.contains(qext))
                    formats.append(qext);
            }
        }
        return formats;
    }();
    return extensions;
}

OIIOWriter::OIIOWriter(QObject* object)
    : plugins::MediaWriter(object)
    , p(new OIIOWriterPrivate())
{}

OIIOWriter::~OIIOWriter() {}

bool
OIIOWriter::open(const core::File& file, const Options& options)
{
    p->d.file = file;
    return true;
}

bool
OIIOWriter::close()
{}

bool
OIIOWriter::isOpen() const
{
    return true;
}

bool
OIIOWriter::supportsImage() const
{
    return true;
}

bool
OIIOWriter::supportsAudio() const
{
    return false;
}

QList<QString>
OIIOWriter::extensions() const
{
    return p->extensions();
}

av::Time
OIIOWriter::write(const core::ImageBuffer& image)
{
    return p->write(image);
}

av::Time
OIIOWriter::seek(const av::TimeRange& timerange)
{
    p.detach();
    p->d.timerange = timerange;
    p->d.timestamp = p->d.timerange.start();
    return p->d.timestamp;
}

av::Time
OIIOWriter::time() const
{
    return p->d.timestamp;
}

av::Fps
OIIOWriter::fps() const
{
    return p->d.fps;
}

av::TimeRange
OIIOWriter::timeRange() const
{
    return p->d.timerange;
}

core::Error
OIIOWriter::error() const
{
    return p->d.error;
}

void
OIIOWriter::setFps(const av::Fps& fps)
{
    p->d.fps = fps;
}

void
OIIOWriter::setTimeRange(const av::TimeRange& timeRange)
{
    p.detach();
    seek(timeRange);
}

bool
OIIOWriter::setMetaData(const core::MetaData& metaData)
{
    p.detach();
    p->d.metaData = metaData;
}

plugins::PluginHandler
OIIOWriter::handler()
{
    static plugins::PluginHandler handler = plugins::PluginHandler::create<MediaWriter>(OIIOWriterPrivate::info(),
                                                                                        OIIOWriterPrivate::extensions,
                                                                                        OIIOWriterPrivate::creator);
    return handler;
}

}  // namespace flipman::sdk::plugins
