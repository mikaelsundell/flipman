// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/filerange.h>
#include <flipmansdk/plugins/oiio/oiioreader.h>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/typedesc.h>

using namespace OIIO;

namespace flipman::sdk::plugins {

class OIIOReaderPrivate : public QSharedData {
public:
    OIIOReaderPrivate();
    ~OIIOReaderPrivate();
    bool open(const core::File& file, const OIIOReader::Options& options);
    bool close();
    av::Time read();
    av::Time skip();
    av::Time seek(const av::TimeRange& range);
    core::ImageFormat::Type toImageType(const OIIO::TypeDesc& type);
    static plugins::PluginHandler::Info info();
    static core::Plugin* creator();
    static QList<QString> extensions();
    struct Data {
        core::File file;
        std::unique_ptr<OIIO::ImageInput> input;
        QString fileName;
        av::Fps fps = av::Fps::fps24();
        av::TimeRange timeRange;
        av::Time startStamp;
        av::Time timeStamp;
        core::ImageBuffer image;
        core::MetaData metaData;
        bool open = false;
        core::Error error;
    };
    Data d;
};

namespace {
    std::once_flag flag;
    void init()
    {
        int threads = 0;
        OIIO::attribute("exr_threads", threads);
    }
}  // namespace

OIIOReaderPrivate::OIIOReaderPrivate() { std::call_once(flag, init); }

OIIOReaderPrivate::~OIIOReaderPrivate() {}

bool
OIIOReaderPrivate::open(const core::File& file, const OIIOReader::Options& options)
{
    Q_UNUSED(options);

    d.file = file;

    const core::FileRange range = file.fileRange();

    d.startStamp = av::Time::zero(d.fps);
    d.timeStamp = d.startStamp;

    if (range.isValid()) {
        const qint64 count = range.size();
        d.timeRange = av::TimeRange(d.startStamp, av::Time::fromFrames(count, d.fps));
    }
    else {
        d.timeRange = av::TimeRange(d.startStamp, av::Time::fromFrames(1, d.fps));
    }


    //qDebug() << d.timeRange.toString();

    QString fileName;

    if (range.isValid())
        fileName = range.frame(range.start()).filePath();
    else
        fileName = file.filePath();

    d.input = OIIO::ImageInput::open(fileName.toStdString());

    if (!d.input) {
        d.error = core::Error("oiioreader", "could not open image");
        return false;
    }

    d.fileName = fileName;
    d.open = true;

    return true;
}

bool
OIIOReaderPrivate::close()
{
    if (d.input) {
        d.input->close();
        d.input.reset();
    }

    d.fileName.clear();
    d.open = false;

    return true;
}

av::Time
OIIOReaderPrivate::read()
{
    if (!d.open) {
        d.error = core::Error("oiioreader", "reader not open");
        return d.timeStamp;
    }

    const core::FileRange range = d.file.fileRange();
    const qint64 timelineFrame = d.timeStamp.frames();

    QString fileName;

    if (range.isValid()) {
        const qint64 fileFrame = range.start() + timelineFrame;

        if (!range.hasFrame(fileFrame)) {
            d.error = core::Error("oiioreader", "frame not mapped");
            return d.timeStamp;
        }

        fileName = range.frame(fileFrame).filePath();
    }
    else {
        fileName = d.file.filePath();
    }

    qDebug() << "open:" << fileName;

    if (!d.input || d.fileName != fileName) {
        if (d.input)
            d.input->close();

        d.input = OIIO::ImageInput::open(fileName.toStdString());

        if (!d.input) {
            d.error = core::Error("oiioreader", "could not open frame");
            return d.timeStamp;
        }

        d.fileName = fileName;
    }

    const OIIO::ImageSpec& spec = d.input->spec();

    const int width = spec.width;
    const int height = spec.height;
    const int channels = spec.nchannels;

    const core::ImageFormat::Type type = toImageType(spec.format);

    if (type == core::ImageFormat::Type::Unknown) {
        d.error = core::Error("oiioreader", "unsupported pixel format");
        return d.timeStamp;
    }

    core::ImageFormat format(type);

    QRect dataWindow(0, 0, width, height);
    QRect displayWindow = dataWindow;

    d.image = core::ImageBuffer(dataWindow, displayWindow, format, channels);

    bool ok = d.input->read_image(0, 0, 0, channels, spec.format, d.image.data(), OIIO::AutoStride, OIIO::AutoStride,
                                  OIIO::AutoStride);

    if (!ok) {
        std::string err = d.input->geterror();

        d.error = core::Error("oiioreader", err.c_str());

        return d.timeStamp;
    }

    av::Time current = d.timeStamp;

    d.timeStamp.setTicks(d.timeStamp.ticks() + d.timeStamp.tpf());

    return current;
}

av::Time
OIIOReaderPrivate::skip()
{
    av::Time current = d.timeStamp;
    d.timeStamp.setTicks(d.timeStamp.ticks() + d.timeStamp.tpf());
    return current;
}

av::Time
OIIOReaderPrivate::seek(const av::TimeRange& range)
{
    d.timeRange = range;
    d.startStamp = range.start();
    d.timeStamp = d.startStamp;
    if (d.input)
        d.input->close();
    d.input.reset();
    return d.timeStamp;
}

core::ImageFormat::Type
OIIOReaderPrivate::toImageType(const OIIO::TypeDesc& type)
{
    using T = core::ImageFormat::Type;

    if (type == OIIO::TypeDesc::UINT8)
        return T::UInt8;
    if (type == OIIO::TypeDesc::INT8)
        return T::Int8;

    if (type == OIIO::TypeDesc::UINT16)
        return T::UInt16;
    if (type == OIIO::TypeDesc::INT16)
        return T::Int16;

    if (type == OIIO::TypeDesc::UINT32)
        return T::UInt32;
    if (type == OIIO::TypeDesc::INT32)
        return T::Int32;

    if (type == OIIO::TypeDesc::UINT64)
        return T::UInt64;
    if (type == OIIO::TypeDesc::INT64)
        return T::Int64;

    if (type == OIIO::TypeDesc::HALF)
        return T::Half;
    if (type == OIIO::TypeDesc::FLOAT)
        return T::Float;
    if (type == OIIO::TypeDesc::DOUBLE)
        return T::Double;

    return T::Unknown;
}

plugins::PluginHandler::Info
OIIOReaderPrivate::info()
{
    return { "oiioreader", "reads images using OpenImageIO", "1.0.0" };
}

core::Plugin*
OIIOReaderPrivate::creator()
{
    return new OIIOReader();
}

QList<QString>
OIIOReaderPrivate::extensions()
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

OIIOReader::OIIOReader(QObject* parent)
    : plugins::MediaReader(parent)
    , p(new OIIOReaderPrivate())
{}

OIIOReader::~OIIOReader() {}

bool
OIIOReader::open(const core::File& file, const Options& options)
{
    return p->open(file, options);
}

bool
OIIOReader::close()
{
    return p->close();
}

bool
OIIOReader::isOpen() const
{
    return p->d.open;
}

bool
OIIOReader::supportsImage() const
{
    return true;
}

bool
OIIOReader::supportsAudio() const
{
    return true;
}

bool
OIIOReader::supportsConcurrent() const
{
    return true;
}

av::Time
OIIOReader::read()
{
    return p->read();
}

av::Time
OIIOReader::skip()
{
    return p->skip();
}

av::Time
OIIOReader::seek(const av::TimeRange& timerange)
{
    return p->seek(timerange);
}

av::Time
OIIOReader::start() const
{
    return p->d.startStamp;
}

av::Time
OIIOReader::time() const
{
    return p->d.timeStamp;
}

av::Fps
OIIOReader::fps() const
{
    return p->d.fps;
}

av::TimeRange
OIIOReader::timeRange() const
{
    return p->d.timeRange;
}

QList<QString>
OIIOReader::extensions() const
{
    return p->extensions();
}

core::AudioBuffer
OIIOReader::audio() const
{
    return core::AudioBuffer();
}

core::ImageBuffer
OIIOReader::image() const
{
    return p->d.image;
}

core::MetaData
OIIOReader::metaData() const
{
    return p->d.metaData;
}

core::Error
OIIOReader::error() const
{
    return p->d.error;
}

plugins::PluginHandler
OIIOReader::handler()
{
    static plugins::PluginHandler handler = plugins::PluginHandler::create<MediaReader>(OIIOReaderPrivate::info(),
                                                                                        OIIOReaderPrivate::extensions,
                                                                                        OIIOReaderPrivate::creator);
    return handler;
}

}  // namespace flipman::sdk::plugins
