// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderdevice.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderoffscreen.h>

namespace flipman::sdk::render {

class RenderOffscreenPrivate {
public:
    struct Data {
        std::unique_ptr<RenderDevice> device;
        std::unique_ptr<RenderEngine> engine;
        QList<ImageLayer> imageLayers;
        QColor background = Qt::black;
        QSize size;
        bool initialized = false;
    };
    Data d;
};

RenderOffscreen::RenderOffscreen(QObject* parent)
    : QObject(parent)
    , p(new RenderOffscreenPrivate())
{}

RenderOffscreen::~RenderOffscreen() {}

bool
RenderOffscreen::initialize(const QSize& size)
{
    if (size.isEmpty())
        return false;

    p->d.size = size;
    p->d.device = std::make_unique<RenderDevice>();
    if (!p->d.device->create(RenderDevice::Auto, size))
        return false;

    p->d.engine = std::make_unique<RenderEngine>();
    p->d.engine->setResolution(size);
    p->d.engine->setBackground(p->d.background);
    p->d.initialized = true;
    return true;
}

void
RenderOffscreen::setBackground(const QColor& color)
{
    p->d.background = color;
    if (p->d.engine)
        p->d.engine->setBackground(color);
}

void
RenderOffscreen::setImageLayers(const QList<ImageLayer>& imageLayers)
{
    p->d.imageLayers = imageLayers;
    if (p->d.engine)
        p->d.engine->setImageLayers(imageLayers);
}

core::ImageBuffer
RenderOffscreen::render()
{
    if (!p->d.initialized)
        return {};

    QRhiCommandBuffer* commandBuffer = nullptr;

    if (!p->d.device->beginFrame(commandBuffer))
        return {};

    RenderEngine::Context context = p->d.device->context();
    p->d.engine->initialize(context);
    p->d.engine->render(context, commandBuffer);
    p->d.device->endFrame();
    return p->d.device->readback();
}

}  // namespace flipman::sdk::render
