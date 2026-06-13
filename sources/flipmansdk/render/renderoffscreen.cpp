// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderdevice.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderoffscreen.h>
#include <flipmansdk/render/renderspec.h>
#include <QPointer>

namespace flipman::sdk::render {

class RenderOffscreenPrivate {
public:
    struct Data {
        std::unique_ptr<RenderDevice> device;
        QPointer<render::RenderEngine> renderEngine;
        bool initialized = false;
    };

    Data d;
};

RenderOffscreen::RenderOffscreen(QObject* parent)
    : QObject(parent)
    , p(new RenderOffscreenPrivate())
{}

RenderOffscreen::~RenderOffscreen() {}

render::RenderEngine*
RenderOffscreen::renderEngine() const
{
    return p->d.renderEngine;
}

void
RenderOffscreen::setRenderEngine(render::RenderEngine* renderEngine)
{
    if (p->d.renderEngine == renderEngine)
        return;

    p->d.renderEngine = renderEngine;
}

bool
RenderOffscreen::initialize(const QSize& size)
{
    if (size.isEmpty())
        return false;

    p->d.device = std::make_unique<RenderDevice>();

    if (!p->d.device->create(RenderDevice::Auto, size))
        return false;

    p->d.initialized = true;
    return true;
}

core::ImageBuffer
RenderOffscreen::render()
{
    if (!p->d.initialized || !p->d.renderEngine || !p->d.device)
        return {};

    QRhiCommandBuffer* commandBuffer = nullptr;

    if (!p->d.device->beginFrame(commandBuffer))
        return {};

    RenderContext context = p->d.device->context();

    RenderSurface surface;
    surface.setRenderTarget(p->d.device->surface().renderTarget());

    RenderSpec renderSpec;
    renderSpec.setSurface(surface);
    renderSpec.setSize(p->d.device->size());

    QMatrix4x4 view;
    view.setToIdentity();
    renderSpec.setView(view);

    if (!context.isValid() || !renderSpec.isValid()) {
        p->d.device->endFrame();
        return {};
    }

    if (!p->d.renderEngine->initialize(context, renderSpec)) {
        p->d.device->endFrame();
        return {};
    }
    p->d.renderEngine->render(context, renderSpec, commandBuffer);
    p->d.device->endFrame();
    return p->d.device->readback();
}

}  // namespace flipman::sdk::render
