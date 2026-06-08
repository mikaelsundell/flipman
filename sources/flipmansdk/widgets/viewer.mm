// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/widgets/viewer.h>
#include <flipmansdk/core/core.h>
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
#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>
#include <QuartzCore/CAMetalLayer.h>

namespace flipman::sdk::widgets {

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
    CAMetalLayer* findMetalLayer(CALayer* layer);
    QColorSpace::Primaries toPrimaries(render::ColorSpace colorSpace);
    QColorSpace toDisplayColorSpace(render::ColorSpace colorSpace,
                                     render::TransferFunction transferFunction);
    void updateWidget(QWidget* widget, const render::DisplayTransform& transform);
    struct Data {
        std::unique_ptr<render::RenderEngine> renderEngine;
        QList<render::ImageLayer> imageLayers;
        QColor background = core::style()->color(core::Style::Viewer);
        QSize resolution = QSize(1920, 1080);
        render::DisplayTransform displayTransform;
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
    updateWidget(d.widget, d.displayTransform);
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

QColorSpace::Primaries
ViewerPrivate::toPrimaries(render::ColorSpace colorSpace)
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
        qWarning() << "viewer: ACEScg/AP1 primaries cannot be represented by QColorSpace::Primaries,"
                   << "falling back to sRGB primaries for display tagging";
        return QColorSpace::Primaries::SRgb;
    }

    qWarning() << "viewer: unknown color space for display primaries,"
               << "falling back to sRGB primaries"
               << "colorSpace:" << int(colorSpace);

    return QColorSpace::Primaries::SRgb;
}

QColorSpace
ViewerPrivate::toDisplayColorSpace(render::ColorSpace colorSpace,
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

    const QColorSpace::Primaries primaries = toPrimaries(colorSpace);

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

    case render::TransferFunction::SRGB:
        return QColorSpace(primaries, QColorSpace::TransferFunction::SRgb);

    case render::TransferFunction::Auto:
    case render::TransferFunction::Raw:
        qWarning() << "viewer: transfer function cannot be represented as a display color space,"
                   << "falling back to sRGB"
                   << "transferFunction:" << int(transferFunction);
        return QColorSpace(QColorSpace::NamedColorSpace::SRgb);

    case render::TransferFunction::ACEScc:
    case render::TransferFunction::ACEScct:
    case render::TransferFunction::ADX10:
    case render::TransferFunction::ADX16:
    case render::TransferFunction::Cineon:
    case render::TransferFunction::AppleLog:
    case render::TransferFunction::ArriLogC3:
    case render::TransferFunction::ArriLogC4:
    case render::TransferFunction::BmdFilmGen5:
    case render::TransferFunction::DaVinciIntermediate:
    case render::TransferFunction::CanonLog2:
    case render::TransferFunction::CanonLog3:
    case render::TransferFunction::PanasonicVLog:
    case render::TransferFunction::RedLog3G10:
    case render::TransferFunction::SonySLog3:
        qWarning() << "viewer: log/camera transfer function cannot be used as a display color space tag,"
                   << "falling back to sRGB"
                   << "transferFunction:" << int(transferFunction);
        return QColorSpace(QColorSpace::NamedColorSpace::SRgb);
    }

    qWarning() << "viewer: unknown transfer function for display color space,"
               << "falling back to sRGB"
               << "transferFunction:" << int(transferFunction);

    return QColorSpace(QColorSpace::NamedColorSpace::SRgb);
}

void
ViewerPrivate::updateWidget(QWidget* widget, const render::DisplayTransform& transform)
{
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

    const QColorSpace qtColorSpace = toDisplayColorSpace(
        transform.colorSpace,
        transform.transferFunction);

    CGColorSpaceRef cgColorSpace = nullptr;

    if (!qtColorSpace.isValid()) {
        qWarning() << "viewer: failed to create color space for display,"
                   << "falling back to sRGB";

        cgColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    }
    else {
        const QByteArray icc = qtColorSpace.iccProfile();

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
            CFDataRef data = CFDataCreate(
                kCFAllocatorDefault,
                reinterpret_cast<const UInt8*>(icc.constData()),
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

    metalLayer.colorspace = cgColorSpace;
    CGColorSpaceRelease(cgColorSpace);
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

render::DisplayTransform
Viewer::displayTransform() const
{
    return p->d.displayTransform;
}

void
Viewer::setDisplayTransform(const render::DisplayTransform& transform)
{
    if (p->d.displayTransform.colorSpace == transform.colorSpace &&
        p->d.displayTransform.transferFunction == transform.transferFunction) {
        return;
    }

    p->d.displayTransform = transform;
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
