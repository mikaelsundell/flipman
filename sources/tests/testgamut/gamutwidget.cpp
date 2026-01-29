// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include "gamutwidget.h"
#include <QFile>
#include <QMouseEvent>
#include <QPointer>

class GamutWidgetPrivate : public QObject {
    Q_OBJECT
public:
    GamutWidgetPrivate();
    void init();
    void init(QRhiCommandBuffer* cb);
    void render(QRhiCommandBuffer* cb);
    void transform(QRhiCommandBuffer* cb);
    QShader shader(const QString& name);

public:
    struct Data {
        QRhi* rhi = nullptr;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        std::unique_ptr<QRhiBuffer> vertexbuffer;
        std::unique_ptr<QRhiBuffer> mvpbuffer;
        std::unique_ptr<QRhiTexture> texturebuffer;
        std::unique_ptr<QRhiSampler> texturesampler;
        std::unique_ptr<QRhiShaderResourceBindings> shaderresourcebindings;
        float zoom = 1.0f;
        QPoint mousepos;
        QPointF pan;
        QColor background;  // TODO = palette_role(GamutWidget);
        QSize resolution = QSize(1920, 1080);
        core::ImageBuffer imagebuffer;
        QVector<float> vertexdata;
        QMatrix4x4 mvpdata;
    };
    Data d;
    QPointer<GamutWidget> widget;
};

GamutWidgetPrivate::GamutWidgetPrivate() {}

void
GamutWidgetPrivate::init()
{}

void
GamutWidgetPrivate::init(QRhiCommandBuffer* cb)
{
    qDebug() << "init(QRhiCommandBuffer* cb)";

    if (d.rhi != widget->rhi()) {
        d.pipeline.reset();
        d.rhi = widget->rhi();
    }


    // compile shaders here!



    static bool info = true;
    if (info) {
        qDebug() << "Device id: " << d.rhi->driverInfo().deviceId;
        qDebug() << "Device name: " << d.rhi->driverInfo().deviceName;
        switch (d.rhi->driverInfo().deviceType) {
        case QRhiDriverInfo::UnknownDevice: qDebug() << "Device type: unknown device"; break;
        case QRhiDriverInfo::IntegratedDevice: qDebug() << "Device type: integrated device"; break;
        case QRhiDriverInfo::DiscreteDevice: qDebug() << "Device type: discrete device"; break;
        case QRhiDriverInfo::ExternalDevice: qDebug() << "Device type: external device"; break;
        case QRhiDriverInfo::VirtualDevice: qDebug() << "Device type: virtual device"; break;
        case QRhiDriverInfo::CpuDevice: qDebug() << "Device type: cpu device"; break;
        }
        qDebug() << "Vendor id: " << d.rhi->driverInfo().vendorId;
        qDebug() << "Backend: " << d.rhi->backendName();
        info = false;
    }

    if (!d.pipeline) {
        d.mvpbuffer.reset(d.rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                           sizeof(QMatrix4x4) + sizeof(float) * 4));  // alignment padding
        d.mvpbuffer->create();

        d.shaderresourcebindings.reset(d.rhi->newShaderResourceBindings());
        d.shaderresourcebindings->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.mvpbuffer.get()) });
        d.shaderresourcebindings->create();

        d.pipeline.reset(d.rhi->newGraphicsPipeline());
        d.pipeline->setTopology(QRhiGraphicsPipeline::Points);
        d.pipeline->setShaderStages({ { QRhiShaderStage::Vertex, shader("vertex.vert.qsb") },
                                      { QRhiShaderStage::Fragment, shader("fragment.frag.qsb") } });

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 6 * sizeof(float) } });  // 6 floats per vertex
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },                 // position
            { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) }  // color
        });
        d.pipeline->setVertexInputLayout(inputLayout);
        d.pipeline->setShaderResourceBindings(d.shaderresourcebindings.get());
        d.pipeline->setRenderPassDescriptor(widget->renderTarget()->renderPassDescriptor());

        d.pipeline->create();
    }
}

void
GamutWidgetPrivate::render(QRhiCommandBuffer* cb)
{
    if (d.rhi != widget->rhi()) {
        widget->initialize(cb);
    }

    if (!d.imagebuffer.is_valid())
        return;

    const int width = d.imagebuffer.datawindow().width();
    const int height = d.imagebuffer.datawindow().height();
    const int channels = d.imagebuffer.channels();
    if (channels < 3)
        return;

    const float* pixels = reinterpret_cast<const float*>(d.imagebuffer.data());
    QVector<float> pointData;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * channels;

            float r = pixels[idx + 0];
            float g = pixels[idx + 1];
            float b = pixels[idx + 2];
            float px = static_cast<float>(x);
            float py = static_cast<float>(y);
            float pz = 0.0f;

            pointData << px << py << pz << r << g << b;
        }
    }

    int pointCount = width * height;
    int totalSize = pointData.size() * sizeof(float);

    d.vertexbuffer.reset(d.rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, totalSize));
    d.vertexbuffer->create();

    QRhiResourceUpdateBatch* upload = d.rhi->nextResourceUpdateBatch();
    upload->uploadStaticBuffer(d.vertexbuffer.get(), pointData.constData());
    cb->resourceUpdate(upload);

    QSize renderSize = widget->renderTarget()->pixelSize();
    float dpr = widget->devicePixelRatioF();
    float rw = renderSize.width() / dpr;
    float rh = renderSize.height() / dpr;

    float w = d.resolution.width();
    float h = d.resolution.height();
    float x = (rw - w * d.zoom) * 0.5f + d.pan.x();
    float y = (rh - h * d.zoom) * 0.5f + d.pan.y();

    QMatrix4x4 mvp = d.rhi->clipSpaceCorrMatrix();
    mvp.ortho(0.0f, rw, rh, 0.0f, -1.0f, 1.0f);
    mvp.translate(x, y);
    mvp.scale(d.zoom);

    struct Uniform {
        QMatrix4x4 mvp;
        float pointSize;
        float padding[3];  // 16-byte alignment
    } uniform;
    uniform.mvp = mvp;
    uniform.pointSize = 4.0f;

    QRhiResourceUpdateBatch* resourceUpdates = d.rhi->nextResourceUpdateBatch();
    resourceUpdates->updateDynamicBuffer(d.mvpbuffer.get(), 0, sizeof(Uniform), &uniform);
    cb->resourceUpdate(resourceUpdates);

    cb->beginPass(widget->renderTarget(), d.background, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.pipeline.get());

    cb->setViewport(QRhiViewport(0, 0, renderSize.width(), renderSize.height()));

    const QRhiCommandBuffer::VertexInput vertexBinding(d.vertexbuffer.get(), 0);
    cb->setVertexInput(0, 1, &vertexBinding);
    cb->setShaderResources(d.shaderresourcebindings.get());
    cb->draw(pointCount);
    cb->endPass();
}
QShader
GamutWidgetPrivate::shader(const QString& name)
{
    const QString resourcePath = QStringLiteral(":/shaders/") + name;
    QFile file(resourcePath);
    if (file.open(QIODevice::ReadOnly)) {
        QShader shader = QShader::fromSerialized(file.readAll());
        QShaderDescription desc = shader.description();
        const auto bindings = desc.uniformBlocks();
        const auto textures = desc.combinedImageSamplers();  // <--- This is what you want

        qDebug() << "Shader:" << name;
        if (!textures.isEmpty()) {
            qDebug() << "Shader has texture bindings:";
            for (const QShaderDescription::InOutVariable& t : textures) {
                qDebug() << "  Name:" << t.name << "Binding:" << t.binding << "Type:" << t.type
                         << "Location:" << t.location;
            }
        }
        else {
            qDebug() << "No texture bindings found.";
        }

        return shader;


        //return QShader::fromSerialized(file.readAll());
    }
    else {
        qWarning() << "warning: unable to open embedded shader resource:" << resourcePath;
        return QShader();
    }
}

#include "gamutwidget.moc"

GamutWidget::GamutWidget(QWidget* parent)
    : QRhiWidget(parent)
    , p(new GamutWidgetPrivate())
{
    p->widget = this;
    p->init();
}

GamutWidget::~GamutWidget() {}

QSize
GamutWidget::resolution() const
{
    return p->d.resolution;
}

void
GamutWidget::set_background(const QColor& background)
{
    p->d.background = background;
    update();
}

void
GamutWidget::set_image(const core::ImageBuffer& imagebuffer)
{
    p->d.imagebuffer = imagebuffer;
    update();
}

void
GamutWidget::set_resolution(const QSize& resolution)
{
    p->d.resolution = resolution;
    update();
}

void
GamutWidget::initialize(QRhiCommandBuffer* cb)
{
    p->init(cb);
}


void
GamutWidget::render(QRhiCommandBuffer* cb)
{
    p->render(cb);
}

void
GamutWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        p->d.mousepos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void
GamutWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::MiddleButton) {
        QPoint delta = event->pos() - p->d.mousepos;
        p->d.pan += delta;
        p->d.mousepos = event->pos();
        update();
    }
}

void
GamutWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        unsetCursor();
    }
}

void
GamutWidget::wheelEvent(QWheelEvent* event)
{
    constexpr float zoomfactor = 1.1f;
    float oldZoom = p->d.zoom;

    if (event->angleDelta().y() > 0) {
        p->d.zoom *= zoomfactor;
    }
    else {
        p->d.zoom /= zoomfactor;
    }
    p->d.zoom = std::clamp(p->d.zoom, 0.1f, 10.0f);
    update();
}
