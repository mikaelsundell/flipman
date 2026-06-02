// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/widgets/viewer.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>
#include <flipmansdk/render/renderengine.h>
#include <QColorSpace>
#include <QDebug>
#include <QMouseEvent>
#include <QPointer>
#include <algorithm>
#include <cmath>

#include <rhi/qrhi.h>

#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

namespace flipman::sdk::widgets {

#ifdef Q_OS_MACOS
static CAMetalLayer*
findMetalLayer(CALayer* layer)
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

static QColorSpace::Primaries
toQtPrimaries(render::ColorSpace colorSpace)
{
    switch (colorSpace) {
    case render::ColorSpace::Auto:
    case render::ColorSpace::Raw:
    case render::ColorSpace::Rec709:
        return QColorSpace::Primaries::SRgb;
    case render::ColorSpace::DisplayP3:
    case render::ColorSpace::DCIP3:
        return QColorSpace::Primaries::DciP3D65;
    case render::ColorSpace::Rec2020:
        return QColorSpace::Primaries::Bt2020;
    case render::ColorSpace::ACEScg:
        return QColorSpace::Primaries::SRgb;
    }
    return QColorSpace::Primaries::SRgb;
}

static QColorSpace
createQtDisplayColorSpace(render::ColorSpace colorSpace,
                          render::TransferFunction transferFunction)
{
    if (colorSpace == render::ColorSpace::DisplayP3 &&
        transferFunction == render::TransferFunction::SRGB) {
        return QColorSpace(QColorSpace::NamedColorSpace::DisplayP3);
    }

    if ((colorSpace == render::ColorSpace::Rec709 ||
         colorSpace == render::ColorSpace::Auto) &&
        transferFunction == render::TransferFunction::SRGB) {
        return QColorSpace(QColorSpace::NamedColorSpace::SRgb);
    }

    if ((colorSpace == render::ColorSpace::Rec709 ||
         colorSpace == render::ColorSpace::Auto) &&
        transferFunction == render::TransferFunction::Linear) {
        return QColorSpace(QColorSpace::NamedColorSpace::SRgbLinear);
    }

    const QColorSpace::Primaries primaries = toQtPrimaries(colorSpace);

    switch (transferFunction) {
    case render::TransferFunction::Gamma22:
        return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.2f);

    case render::TransferFunction::Gamma24:
        return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.4f);

    case render::TransferFunction::Gamma25:
        return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.5f);

    case render::TransferFunction::Gamma26:
        return QColorSpace(primaries, QColorSpace::TransferFunction::Gamma, 2.6f);

    case render::TransferFunction::Linear:
        return QColorSpace(primaries, QColorSpace::TransferFunction::Linear);

    case render::TransferFunction::Auto:
    case render::TransferFunction::Raw:
    case render::TransferFunction::SRGB:
    case render::TransferFunction::Cineon:
    case render::TransferFunction::ArriLogC3:
    case render::TransferFunction::ArriLogC4:
    case render::TransferFunction::SonySLog3:
    case render::TransferFunction::ACEScct:
        return QColorSpace(primaries, QColorSpace::TransferFunction::SRgb);
    }

    return QColorSpace(QColorSpace::NamedColorSpace::SRgb);
}

static CGColorSpaceRef
createCGColorSpace(render::ColorSpace colorSpace,
                   render::TransferFunction transferFunction)
{
    QColorSpace qtColorSpace = createQtDisplayColorSpace(colorSpace, transferFunction);

    if (!qtColorSpace.isValid())
        return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

    const QByteArray icc = qtColorSpace.iccProfile();

    if (icc.isEmpty()) {
        if (colorSpace == render::ColorSpace::DisplayP3)
            return CGColorSpaceCreateWithName(kCGColorSpaceDisplayP3);

        return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    }

    CFDataRef data = CFDataCreate(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(icc.constData()),
        icc.size());

    if (!data)
        return CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

    CGColorSpaceRef cgColorSpace = CGColorSpaceCreateWithICCData(data);
    CFRelease(data);

    return cgColorSpace;
}

static void
setViewerMetalLayerColorSpace(QWidget* widget,
                              render::ColorSpace colorSpace,
                              render::TransferFunction transferFunction)
{
    if (!widget)
        return;

    NSView* view = reinterpret_cast<NSView*>(widget->winId());
    if (!view)
        return;

    CAMetalLayer* metalLayer = findMetalLayer(view.layer);
    if (!metalLayer)
        return;

    CGColorSpaceRef cgColorSpace = createCGColorSpace(colorSpace, transferFunction);

    if (!cgColorSpace)
        return;

    metalLayer.colorspace = cgColorSpace;
    CGColorSpaceRelease(cgColorSpace);
}

#endif

class ViewerPrivate : public QObject {
public:
    ViewerPrivate();
    void init();
    void clear();
    void render(QRhiCommandBuffer* cb);
    void updateContext(render::RenderEngine::Context& context);
    void updateView();
    void updateDisplayColorSpace();
public:
    struct Data {
        std::unique_ptr<render::RenderEngine> renderEngine;
        QList<render::ImageLayer> imageLayers;
        QColor background = core::style()->color(core::Style::Viewer);
        QSize resolution = QSize(1920, 1080);
        render::ColorSpace displayColorSpace = render::ColorSpace::Rec709;
        render::TransferFunction displayTransferFunction = render::TransferFunction::Gamma24;
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
    d.widget->setZoom(1.0f);
}

void
ViewerPrivate::updateDisplayColorSpace()
{
#ifdef Q_OS_MACOS
    setViewerMetalLayerColorSpace(
        d.widget,
        d.displayColorSpace,
        d.displayTransferFunction);
#endif
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
#ifdef Q_OS_MACOS
    setApi(QRhiWidget::Api::Metal);
#endif

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
    p->updateDisplayColorSpace();
    p->render(commandBuffer);
}

QSize
Viewer::resolution() const
{
    return p->d.resolution;
}

void
Viewer::setResolution(const QSize& resolution)
{
    p->d.resolution = resolution;
    update();
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

render::ColorSpace
Viewer::displayColorSpace() const
{
    return p->d.displayColorSpace;
}

void
Viewer::setDisplayColorSpace(render::ColorSpace colorSpace)
{
    if (p->d.displayColorSpace == colorSpace)
        return;

    p->d.displayColorSpace = colorSpace;
    p->updateDisplayColorSpace();
    update();
}

render::TransferFunction
Viewer::displayTransferFunction() const
{
    return p->d.displayTransferFunction;
}

void
Viewer::setDisplayTransferFunction(render::TransferFunction transferFunction)
{
    if (p->d.displayTransferFunction == transferFunction)
        return;

    p->d.displayTransferFunction = transferFunction;
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
    if (p->d.zoomMode == mode)
        return;

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

} // namespace flipman::sdk::widgets
