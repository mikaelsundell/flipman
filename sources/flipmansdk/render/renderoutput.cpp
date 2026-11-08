// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QDir>
#include <QScopedPointer>
#include <QStandardPaths>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/log.h>
#include <flipmansdk/plugins/mediawriter.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/render/renderoutput.h>


namespace flipman::sdk::render {

class RenderOutputPrivate : public QSharedData {
public:
    RenderOutputPrivate();
    ~RenderOutputPrivate();

public:
    struct Data {
        bool enabled = false;
        RenderOutput::Format format = RenderOutput::Format::RGBA16F;
        RenderSpec spec;
    };
    Data d;
};

RenderOutputPrivate::RenderOutputPrivate() {}

RenderOutputPrivate::~RenderOutputPrivate() {}

RenderOutput::RenderOutput(QObject* parent)
    : QObject(parent)
    , p(new RenderOutputPrivate())
{}

RenderOutput::~RenderOutput() = default;

bool
RenderOutput::enabled() const
{
    return p->d.enabled;
}

void
RenderOutput::setEnabled(bool enabled)
{
    if (p->d.enabled != enabled) {
        p->d.enabled = enabled;
    }
}

RenderOutput::Format
RenderOutput::format() const
{
    return p->d.format;
}

void
RenderOutput::setFormat(Format format)
{
    if (p->d.format != format) {
        p->d.format = format;
    }
}

RenderSpec
RenderOutput::pass() const
{
    return p->d.spec;
}

void
RenderOutput::setRenderSpec(const RenderSpec& spec)
{
    if (p->d.spec != spec) {
        p->d.spec = spec;
    }
}

void
RenderOutput::enqueueFrame(const core::ImageBuffer& image, qint64 frame)
{
    if (!core::renderLog().isDebugEnabled())
        return;
    qCDebug(flipman::sdk::core::renderLog)
        << "renderoutput: enqueueFrame"
        << "frame" << frame << "valid" << image.isValid() << "allocated" << image.isAllocated() << "dataWindow"
        << image.dataWindow() << "displayWindow" << image.displayWindow() << "format" << int(image.imageFormat().type())
        << "channels" << image.channels() << "packing" << int(image.packing()) << "subsampling"
        << int(image.subsampling()) << "stride" << image.strideSize() << "bytes" << image.byteSize() << "data"
        << static_cast<const void*>(image.data());

    static bool written = false;
    if (written)
        return;

    if (!image.isValid() || !image.isAllocated())
        return;

    written = true;

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString filename = QDir(dir).filePath(QStringLiteral("flipman-renderoutput-rgba8.png"));

    QFileInfo output(filename);

    QScopedPointer<plugins::MediaWriter> writer(
        core::pluginRegistry()->getPlugin<plugins::MediaWriter>(output.suffix()));

    if (!writer) {
        core::logErr() << "renderoutput: no writer found for extension:" << output.suffix() << Qt::endl;
        return;
    }

    if (!writer->open(output)) {
        core::logErr() << "renderoutput: could not open writer for file:" << output.filePath() << Qt::endl;
        return;
    }

    if (!writer->write(image)) {
        core::logErr() << "renderoutput: could not write image:" << output.filePath() << Qt::endl;
        return;
    }

    qCDebug(flipman::sdk::core::renderLog) << "renderoutput: wrote debug image" << output.filePath();
}

}  // namespace flipman::sdk::render
