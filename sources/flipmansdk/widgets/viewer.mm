// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/core.h>
#include <flipmansdk/core/ui.h>
#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/render/renderspec.h>
#include <flipmansdk/widgets/viewer.h>
#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>
#include <QuartzCore/CAMetalLayer.h>
#include <QDebug>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPointer>
#include <rhi/qrhi.h>
#include <algorithm>
#include <cmath>

namespace flipman::sdk::widgets {

class ViewerPrivate : public QObject {
public:
    ViewerPrivate();
    ~ViewerPrivate() override;

    void init();
    void clear();
    void render(QRhiCommandBuffer* commandBuffer);
    void updateContext(render::RenderContext& context);
    render::RenderSpec renderSpec() const;
    void updateView();
    void updateDisplayColorSpace();

public:
    CAMetalLayer* findMetalLayer(CALayer* layer);
    void updateWidget(QWidget* widget, const render::DisplayTransform& transform);

    struct Data {
        // Owns one CoreGraphics reference until the display profile changes
        // or the viewer is destroyed; CAMetalLayer retains its own reference.
        CGColorSpaceRef nativeColorSpace = nullptr;
        Viewer::ZoomMode zoomMode = Viewer::FitToView;
        render::DisplayTransform displayTransform;
        render::DisplayColorSpace displayColorSpace { render::DisplayTransform() };
        QMatrix4x4 view;
        QSize viewSize;
        QSize viewResolution;
        QPointF mousepos;
        QPointF pan;
        QPointer<render::RenderEngine> renderEngine;
        float zoom = 1.0f;
        QPointer<Viewer> widget;
    };
    Data d;
};

ViewerPrivate::ViewerPrivate() {}

ViewerPrivate::~ViewerPrivate()
{
    if (d.nativeColorSpace)
        CGColorSpaceRelease(d.nativeColorSpace);
}

void
ViewerPrivate::init()
{
    d.zoomMode = Viewer::Manual;
    clear();
}

void
ViewerPrivate::clear()
{
    d.view.setToIdentity();
    d.mousepos = QPointF();
    d.pan = QPointF();
    d.widget->setZoom(1.0f);
}

void
ViewerPrivate::updateDisplayColorSpace()
{
    updateWidget(d.widget, d.displayTransform);
}

void
ViewerPrivate::updateContext(render::RenderContext& context)
{
    context.setRhi(d.widget ? d.widget->rhi() : nullptr);
    render::RenderSurface surface;
    if (d.widget)
        surface.setRenderTarget(d.widget->renderTarget());

    context.setSurface(surface);
}

render::RenderSpec
ViewerPrivate::renderSpec() const
{
    render::RenderSpec spec;

    if (!d.widget)
        return spec;

    QRhiRenderTarget* renderTarget = d.widget->renderTarget();
    if (!renderTarget)
        return spec;

    spec.setView(d.view);
    spec.setSize(renderTarget->pixelSize());

    return spec;
}

void
ViewerPrivate::updateView()
{
    d.view.setToIdentity();

    if (!d.widget || !d.renderEngine)
        return;

    auto* renderTarget = d.widget->renderTarget();
    if (!renderTarget)
        return;

    const QSize widgetSize = renderTarget->pixelSize();
    const QSize resolution = d.renderEngine->resolution();

    if (!widgetSize.isValid() || resolution.isEmpty())
        return;

    d.viewSize = widgetSize;
    d.viewResolution = resolution;

    const float w = float(widgetSize.width());
    const float h = float(widgetSize.height());

    const float imageW = float(resolution.width());
    const float imageH = float(resolution.height());

    if (d.widget->zoomMode() == Viewer::FitToView) {
        const float sx = w / imageW;
        const float sy = h / imageH;
        const float s = std::min(sx, sy);

        d.view.scale(s);

        if (!qFuzzyCompare(d.zoom, s)) {
            d.zoom = s;
            Q_EMIT d.widget->zoomChanged(d.zoom);
        }
    }
    else {
        const float dpr = d.widget->devicePixelRatioF();
        const float panX = ((d.pan.x() * dpr) / w) * 2.0f;
        const float panY = -((d.pan.y() * dpr) / h) * 2.0f;

        d.view.translate(panX, panY);
        d.view.scale(d.zoom);
    }
}

CAMetalLayer*
ViewerPrivate::findMetalLayer(CALayer* layer)
{
    if (!layer)
        return nil;

    if ([layer isKindOfClass:CAMetalLayer.class])
        return static_cast<CAMetalLayer*>(layer);

    for (CALayer* sublayer in layer.sublayers) {
        if (CAMetalLayer* metalLayer = findMetalLayer(sublayer))
            return metalLayer;
    }

    return nil;
}

void
ViewerPrivate::updateWidget(QWidget* widget, const render::DisplayTransform& transform)
{
    // Only Cocoa WIds are NSView pointers. Headless Qt surfaces have no Metal
    // presentation layer to tag, even when their renderer uses a Metal device.
    if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
        return;

    if (!widget) {
        qWarning() << "viewer: failed to update display color space, widget is null";
        return;
    }

    NSView* view = reinterpret_cast<NSView*>(widget->winId());
    if (!view) {
        qWarning() << "viewer: failed to get view from widget";
        return;
    }

    CAMetalLayer* metalLayer = findMetalLayer(view.layer);
    if (!metalLayer) {
        qWarning() << "viewer: failed to get metal layer from view";
        return;
    }

    if (d.nativeColorSpace) {
        metalLayer.colorspace = d.nativeColorSpace;
        return;
    }

    CGColorSpaceRef cgColorSpace = nullptr;

    if (!d.displayColorSpace.isValid()) {
        qWarning() << "viewer: failed to create color space for display,"
                   << "falling back to sRGB";

        cgColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    }
    else {
        const QByteArray icc = d.displayColorSpace.iccProfile();

        if (icc.isEmpty()) {
            if (transform.colorSpace == render::ColorSpace::DisplayP3) {
                qWarning() << "viewer: no ICC profile available,"
                           << "falling back to native DisplayP3 CGColorSpace";
                cgColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceDisplayP3);
            }
            else {
                qWarning() << "viewer: no ICC profile available,"
                           << "falling back to sRGB CGColorSpace";
                cgColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
            }
        }
        else {
            CFDataRef data = CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(icc.constData()),
                                          icc.size());

            if (!data) {
                qWarning() << "viewer: failed to create ICC data for display,"
                           << "falling back to sRGB CGColorSpace";

                cgColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
            }
            else {
                cgColorSpace = CGColorSpaceCreateWithICCData(data);
                CFRelease(data);

                if (!cgColorSpace) {
                    qWarning() << "viewer: failed to create CGColorSpace from ICC,"
                               << "falling back to sRGB CGColorSpace";
                    cgColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
                }
            }
        }
    }

    if (!cgColorSpace) {
        qWarning() << "viewer: failed to create fallback CGColorSpace";
        return;
    }

    d.nativeColorSpace = cgColorSpace;
    metalLayer.colorspace = d.nativeColorSpace;
}

void
ViewerPrivate::render(QRhiCommandBuffer* commandBuffer)
{
    if (!d.renderEngine)
        return;

    render::RenderContext context;
    updateContext(context);

    // The QRhi target may be created/resized after the widget resize event.
    // Refresh geometry only on size changes, including the first native frame.
    if (d.widget->renderTarget()
        && (d.viewSize != d.widget->renderTarget()->pixelSize() || d.viewResolution != d.renderEngine->resolution()))
        updateView();
    const render::RenderSpec spec = renderSpec();

    if (!context.isValid() || !spec.isValid())
        return;

    if (!d.renderEngine->initialize(context, spec))
        return;

    d.renderEngine->render(context, spec, commandBuffer);
}

Viewer::Viewer(QWidget* parent)
    : QRhiWidget(parent)
    , p(new ViewerPrivate())
{
    setApi(QRhiWidget::Api::Metal);
    p->d.widget = this;
    p->init();
}

Viewer::~Viewer() {}

void
Viewer::initialize(QRhiCommandBuffer*)
{
    p->updateDisplayColorSpace();
}

void
Viewer::render(QRhiCommandBuffer* commandBuffer)
{
    p->render(commandBuffer);
}

render::RenderEngine*
Viewer::renderEngine() const
{
    return p->d.renderEngine;
}

void
Viewer::setRenderEngine(render::RenderEngine* renderEngine)
{
    if (p->d.renderEngine == renderEngine)
        return;

    p->d.renderEngine = renderEngine;
    p->updateView();
    update();
}

render::DisplayTransform
Viewer::displayTransform() const
{
    return p->d.displayTransform;
}

render::DisplayColorSpace
Viewer::displayColorSpace() const
{
    return p->d.displayColorSpace;
}

void
Viewer::setDisplayTransform(const render::DisplayTransform& transform)
{
    if (p->d.displayTransform.colorSpace == transform.colorSpace
        && p->d.displayTransform.transferFunction == transform.transferFunction) {
        return;
    }

    p->d.displayTransform = transform;
    p->d.displayColorSpace = render::DisplayColorSpace(transform);
    if (p->d.nativeColorSpace) {
        CGColorSpaceRelease(p->d.nativeColorSpace);
        p->d.nativeColorSpace = nullptr;
    }
    p->updateDisplayColorSpace();
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
    p->d.zoomMode = mode;

    if (mode == FitToView)
        p->d.pan = QPointF();

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
    if (event->button() == Qt::MiddleButton)
        unsetCursor();
}

void
Viewer::wheelEvent(QWheelEvent* event)
{
    if (p->d.zoomMode == Viewer::FitToView)
        setZoomMode(Viewer::Manual);

    const QPointF pos = event->position() - QPointF(width() * 0.5, height() * 0.5);

    const float oldZoom = p->d.zoom;
    const float steps = event->angleDelta().y() / 120.0f;
    const float factor = std::pow(1.1f, steps);
    const float newZoom = std::clamp(oldZoom * factor, 0.01f, 100.0f);

    if (qFuzzyCompare(oldZoom, newZoom)) {
        event->accept();
        return;
    }

    p->d.pan = pos + QPointF((p->d.pan - pos) * (newZoom / oldZoom));
    p->d.zoom = newZoom;

    p->updateView();

    Q_EMIT zoomChanged(p->d.zoom);

    update();
    event->accept();
}

void
Viewer::resizeEvent(QResizeEvent* event)
{
    QRhiWidget::resizeEvent(event);

    p->updateDisplayColorSpace();

    if (zoomMode() == FitToView) {
        p->updateView();
        update();
    }
}

}  // namespace flipman::sdk::widgets
