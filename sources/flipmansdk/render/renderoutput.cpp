// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

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
    qDebug() << "enqueueFrame called with image" << image.displayWindow();
}

}  // namespace flipman::sdk::render
