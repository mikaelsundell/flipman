// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/widgets/viewer.h>

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>
#include <flipmansdk/render/renderengine.h>

#include <QMouseEvent>
#include <QPointer>
#include <algorithm>
#include <rhi/qrhi.h>

namespace flipman::sdk::widgets {

class ViewerPrivate : public QObject {
public:
    ViewerPrivate();
    void init();
    void clear();
    void render(QRhiCommandBuffer* cb);
    void updateContext(render::RenderEngine::Context& context);
    void updateView();

public:
    struct Data {
        std::unique_ptr<render::RenderEngine> renderEngine;
        QList<render::ImageLayer> imageLayers;
        QColor background = core::style()->color(core::Style::Viewer);
        QSize resolution = QSize(1920, 1080);
        QMatrix4x4 view;
        QPointF mousepos;
        QPointF pan;
        Viewer::ZoomMode zoomMode = Viewer::FitToView;
        float zoom = 1.0f;
        QPointer<Viewer> widget;
    };
    Data d;
};

ViewerPrivate::ViewerPrivate() {}

void
ViewerPrivate::init()
{
    d.renderEngine = std::make_unique<render::RenderEngine>(d.widget);
    d.zoomMode = Viewer::Manual;
    clear();
}

void
ViewerPrivate::clear()
{
    d.view.setToIdentity();
    d.mousepos = QPointF();
    d.pan = QPointF();
    d.widget->setZoom(1.0);
}

void
ViewerPrivate::updateContext(render::RenderEngine::Context& context)
{
    auto* renderTarget = d.widget->renderTarget();
    if (!renderTarget)
        return;

    context.rhi = d.widget->rhi();
    context.renderTarget = renderTarget;
    context.renderPassDescriptor = renderTarget->renderPassDescriptor();
    context.view = d.view;
    context.size = renderTarget->pixelSize();
}

void
ViewerPrivate::updateView()
{
    d.view.setToIdentity();

    QSize widgetSize = d.widget->renderTarget()->pixelSize();
    if (!widgetSize.isValid() || d.resolution.isEmpty())
        return;

    const float w = float(widgetSize.width());
    const float h = float(widgetSize.height());

    const float imageW = float(d.resolution.width());
    const float imageH = float(d.resolution.height());

    if (d.widget->zoomMode() == Viewer::FitToView) {
        float sx = w / imageW;
        float sy = h / imageH;
        float s = std::min(sx, sy);
        d.view.scale(s);
        if (!qFuzzyCompare(d.zoom, s)) {
            d.zoom = s;
            Q_EMIT d.widget->zoomChanged(d.zoom);
        }
    }
    else {
        const float dpr = d.widget->devicePixelRatioF();

        float panX = ((d.pan.x() * dpr) / w) * 2.0f;
        float panY = -((d.pan.y() * dpr) / h) * 2.0f;

        d.view.translate(panX, panY);
        d.view.scale(d.zoom);
    }
}

void
ViewerPrivate::render(QRhiCommandBuffer* commandBuffer)
{
    if (!d.renderEngine)
        return;
    render::RenderEngine::Context context;
    updateContext(context);
    if (!context.isValid())
        return;
    d.renderEngine->setBackground(d.background);
    d.renderEngine->setResolution(d.resolution);
    d.renderEngine->setImageLayers(d.imageLayers);
    if (!d.renderEngine->initialize(context))
        return;
    d.renderEngine->render(context, commandBuffer);
}

Viewer::Viewer(QWidget* parent)
    : QRhiWidget(parent)
    , p(new ViewerPrivate())
{
    p->d.widget = this;
    p->init();
}

Viewer::~Viewer() {}

void
Viewer::initialize(QRhiCommandBuffer*)
{}

void
Viewer::render(QRhiCommandBuffer* commandBuffer)
{
    p->render(commandBuffer);
}


QSize
Viewer::resolution() const
{
    return p->d.resolution;
}

void
Viewer::setBackground(const QColor& background)
{
    p->d.background = background;
    update();
}

void
Viewer::setImageLayers(const QList<render::ImageLayer>& imageLayers)
{
    p->d.imageLayers = imageLayers;
    update();
}

void
Viewer::setResolution(const QSize& resolution)
{
    p->d.resolution = resolution;
    update();
}

Viewer::ZoomMode
Viewer::zoomMode() const
{
    return p->d.zoomMode;
}

void
Viewer::setZoom(float zoom)
{
    setZoomMode(Viewer::Manual);
    const float newZoom = std::clamp(zoom, 0.01f, 100.0f);

    if (qFuzzyCompare(p->d.zoom, newZoom))
        return;

    p->d.zoom = newZoom;
    p->updateView();

    Q_EMIT zoomChanged(p->d.zoom);
    update();
}

void
Viewer::setZoomMode(ZoomMode mode)
{
    if (p->d.zoomMode == mode)
        return;

    p->d.zoomMode = mode;

    if (mode == FitToView) {
        p->d.pan = QPointF();
    }

    p->updateView();
    update();
}

float
Viewer::zoom() const
{
    return p->d.zoom;
}

void
Viewer::fitToView()
{
    p->clear();
    update();
}

void
Viewer::resetView()
{
    p->clear();
    update();
}

void
Viewer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        setZoomMode(Viewer::Manual);
        p->d.mousepos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void
Viewer::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::MiddleButton) {
        QPointF delta = event->position() - p->d.mousepos;
        p->d.pan += delta;
        p->d.mousepos = event->position();
        p->updateView();
        update();
    }
}

void
Viewer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        unsetCursor();
    }
}

void
Viewer::wheelEvent(QWheelEvent* event)
{
    QPointF pos = event->position() - rect().center();
    float zoom = p->d.zoom;
    float steps = event->angleDelta().y() / 120.0f;
    float factor = std::pow(1.1f, steps);
    zoom = std::clamp(zoom * factor, 0.01f, 100.0f);
    p->d.pan = pos + QPointF(QPointF(p->d.pan - pos) * (zoom / p->d.zoom));
    p->d.zoom = zoom;
    p->updateView();
    Q_EMIT zoomChanged(p->d.zoom);
    update();
    event->accept();
}

void
Viewer::resizeEvent(QResizeEvent* e)
{
    QRhiWidget::resizeEvent(e);

    if (zoomMode() == FitToView) {
        p->updateView();
        update();
    }
}

}  // namespace flipman::sdk::widgets
