// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/widgets/renderwidget.h>

#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>

#include <QFile>
#include <QMouseEvent>
#include <QPointer>

namespace flipman::sdk::widgets {

class RenderWidgetPrivate : public QObject {
public:
    RenderWidgetPrivate();
    void init();
    void init(QRhiCommandBuffer* cb);
    void render(QRhiCommandBuffer* cb);
    void transform(QRhiCommandBuffer* cb);
    QShader shader(const QString& name);

public:
    struct LayerRenderItem {
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiBuffer> vertexBuffer;
        std::unique_ptr<QRhiShaderResourceBindings> bindings;
    };
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
        QColor background = core::style()->color(core::Style::Viewer);
        QSize resolution = QSize(1920, 1080);
        QList<av::RenderLayer> renderlayers;
        QVector<float> vertexdata;
        QMatrix4x4 mvpdata;
    };
    Data d;
    QPointer<RenderWidget> widget;
};

RenderWidgetPrivate::RenderWidgetPrivate() {}

void
RenderWidgetPrivate::init()
{}

void
RenderWidgetPrivate::init(QRhiCommandBuffer* cb)
{
    qDebug() << "init(QRhiCommandBuffer* cb)";

    if (d.rhi != widget->rhi()) {
        d.pipeline.reset();
        d.rhi = widget->rhi();
    }
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
        d.vertexbuffer.reset(d.rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                              4 * 5 * sizeof(float)));  // 4 vertices, 5 floats per vertex
        d.vertexbuffer->create();
        d.mvpbuffer.reset(d.rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(float) * 16));
        d.mvpbuffer->create();
        d.texturesampler.reset(d.rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                                 QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        d.texturesampler->create();
        d.shaderresourcebindings.reset(d.rhi->newShaderResourceBindings());
        d.shaderresourcebindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.mvpbuffer.get()),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, nullptr,
                                                      d.texturesampler.get())  // texture will be set later
        });
        d.shaderresourcebindings->create();

        d.pipeline.reset(d.rhi->newGraphicsPipeline());
        d.pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        d.pipeline->setShaderStages({ { QRhiShaderStage::Vertex, shader("vertex.vert.qsb") },
                                      { QRhiShaderStage::Fragment, shader("fragment.frag.qsb") } });

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 5 * sizeof(float) } });
        inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
                                    { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) } });

        d.pipeline->setVertexInputLayout(inputLayout);
        d.pipeline->setShaderResourceBindings(d.shaderresourcebindings.get());
        d.pipeline->setRenderPassDescriptor(widget->renderTarget()->renderPassDescriptor());
        d.pipeline->create();
    }
}

static std::unique_ptr<QRhiBuffer>
make_centered_quad(QRhi* rhi, const QSize& layerSize, const QSize& fullSize, QVector<float>& outVertexData)
{
    float w = static_cast<float>(layerSize.width());
    float h = static_cast<float>(layerSize.height());
    float fw = static_cast<float>(fullSize.width());
    float fh = static_cast<float>(fullSize.height());

    float x = (fw - w) * 0.5f;
    float y = (fh - h) * 0.5f;

    outVertexData = { x,     y, 0.0f, 0.0f, 0.0f, x,     y + h, 0.0f, 0.0f, 1.0f,
                      x + w, y, 0.0f, 1.0f, 0.0f, x + w, y + h, 0.0f, 1.0f, 1.0f };

    auto vertexBuffer = std::unique_ptr<QRhiBuffer>(
        rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, outVertexData.size() * sizeof(float)));
    if (!vertexBuffer->create()) {
        qWarning() << "Failed to create vertex buffer for layer quad";
        return nullptr;
    }

    return vertexBuffer;
}

void
RenderWidgetPrivate::render(QRhiCommandBuffer* cb)
{
    if (d.rhi != widget->rhi()) {
        widget->initialize(cb);
    }

    transform(cb);

    QRhiResourceUpdateBatch* resourceUpdates = d.rhi->nextResourceUpdateBatch();
    resourceUpdates->updateDynamicBuffer(d.mvpbuffer.get(), 0, sizeof(float) * 16, d.mvpdata.constData());

    std::vector<LayerRenderItem> layers;

    if (!d.renderlayers.isEmpty()) {
        for (const av::RenderLayer& layer : std::as_const(d.renderlayers)) {
            const core::ImageBuffer& original = layer.image();
            if (!original.isValid())
                continue;

            core::ImageBuffer image = original;
            if (image.channels() != 4 || image.imageFormat().type() != core::ImageFormat::UINT8) {
                image = core::ImageBuffer::convert(original, core::ImageFormat::UINT8, 4);
            }

            quint8* data = image.data();
            if (!data)
                continue;

            QSize texSize = image.displayWindow().size();

            const bool useDummy = true;
            if (useDummy) {
                const int w = 64, h = 64, channels = 4;
                const int size = w * h * channels;
                std::vector<quint8> dummy(size);
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        bool white = ((x / 8) % 2) == ((y / 8) % 2);
                        int i = (y * w + x) * channels;
                        dummy[i + 0] = white ? 255 : 0;
                        dummy[i + 1] = white ? 255 : 0;
                        dummy[i + 2] = white ? 255 : 0;
                        dummy[i + 3] = 255;
                    }
                }
                texSize = QSize(w, h);
                data = dummy.data();
                image = {};  // discard original image to avoid confusion
            }

            std::unique_ptr<QRhiTexture> texture(d.rhi->newTexture(QRhiTexture::RGBA8, texSize, 1));
            if (!texture->create()) {
                qWarning() << "Failed to create texture";
                continue;
            }

            QRhiTextureSubresourceUploadDescription subdesc;
            subdesc.setData(
                QByteArray::fromRawData(reinterpret_cast<const char*>(data), texSize.width() * texSize.height() * 4));
            subdesc.setSourceSize(texSize);

            QRhiTextureUploadEntry entry(0, 0, subdesc);
            QRhiTextureUploadDescription uploadDesc;
            uploadDesc.setEntries({ entry });

            QRhiResourceUpdateBatch* uploadBatch = d.rhi->nextResourceUpdateBatch();
            uploadBatch->uploadTexture(texture.get(), uploadDesc);
            cb->resourceUpdate(uploadBatch);

            QVector<float> quadData;
            std::unique_ptr<QRhiBuffer> vertexBuffer = make_centered_quad(d.rhi, texSize, d.resolution, quadData);
            if (!vertexBuffer)
                continue;

            QRhiResourceUpdateBatch* quadBatch = d.rhi->nextResourceUpdateBatch();
            quadBatch->uploadStaticBuffer(vertexBuffer.get(), quadData.constData());
            cb->resourceUpdate(quadBatch);

            std::unique_ptr<QRhiShaderResourceBindings> bindings(d.rhi->newShaderResourceBindings());
            bindings->setBindings(
                { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.mvpbuffer.get()),
                  QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.get(),
                                                            d.texturesampler.get()) });
            bindings->create();

            layers.push_back({ std::move(texture), std::move(vertexBuffer), std::move(bindings) });
        }
    }

    cb->resourceUpdate(resourceUpdates);
    cb->beginPass(widget->renderTarget(), d.background, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.pipeline.get());

    const QSize viewportSize = widget->renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, viewportSize.width(), viewportSize.height()));

    {
        const QRhiCommandBuffer::VertexInput vertexBinding(d.vertexbuffer.get(), 0);
        cb->setVertexInput(0, 1, &vertexBinding);

        d.shaderresourcebindings->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.mvpbuffer.get()),
              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, nullptr,
                                                        d.texturesampler.get()) });
        d.shaderresourcebindings->create();

        cb->setShaderResources(d.shaderresourcebindings.get());
        cb->draw(4);
    }
    for (const auto& layer : layers) {
        const QRhiCommandBuffer::VertexInput vertexBinding(layer.vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &vertexBinding);
        cb->setShaderResources(layer.bindings.get());
        cb->draw(4);
    }


    cb->endPass();
}

void
RenderWidgetPrivate::transform(QRhiCommandBuffer* cb)
{
    QSize rendersize = widget->renderTarget()->pixelSize();
    float dpr = static_cast<float>(widget->devicePixelRatioF());
    float rw = rendersize.width() / dpr;
    float rh = rendersize.height() / dpr;

    float w = d.resolution.width();
    float h = d.resolution.height();
    float x = (rw - w * d.zoom) * 0.5f + d.pan.x();
    float y = (rh - h * d.zoom) * 0.5f + d.pan.y();

    d.vertexdata
        = { x,    y,    0.0f, 0.0f,           0.0f,           x,    y + h * d.zoom, 0.0f, 0.0f, 1.0f, x + w * d.zoom, y,
            0.0f, 1.0f, 0.0f, x + w * d.zoom, y + h * d.zoom, 0.0f, 1.0f,           1.0f };

    QRhiResourceUpdateBatch* resourceUpdates = d.rhi->nextResourceUpdateBatch();
    resourceUpdates->uploadStaticBuffer(d.vertexbuffer.get(), d.vertexdata.data());
    cb->resourceUpdate(resourceUpdates);
    d.mvpdata = d.rhi->clipSpaceCorrMatrix();
    d.mvpdata.ortho(0.0f, rw, rh, 0.0f, -1.0f, 1.0f);
}

QShader
RenderWidgetPrivate::shader(const QString& name)
{
    const QString resourcePath = QStringLiteral(":/shaders/") + name;
    QFile file(resourcePath);
    if (file.open(QIODevice::ReadOnly)) {
        return QShader::fromSerialized(file.readAll());
    }
    else {
        qWarning() << "warning: unable to open embedded shader resource:" << resourcePath;
        return QShader();
    }
}

RenderWidget::RenderWidget(QWidget* parent)
    : QRhiWidget(parent)
    , p(new RenderWidgetPrivate())
{
    p->widget = this;
    p->init();
}

RenderWidget::~RenderWidget() {}

QSize
RenderWidget::resolution() const
{
    return p->d.resolution;
}

void
RenderWidget::setBackground(const QColor& background)
{
    p->d.background = background;
    update();
}

void
RenderWidget::setRenderLayers(const QList<av::RenderLayer> renderlayers)
{
    p->d.renderlayers = renderlayers;
    update();
}

void
RenderWidget::setResolution(const QSize& resolution)
{
    p->d.resolution = resolution;
    update();
}

void
RenderWidget::initialize(QRhiCommandBuffer* cb)
{
    p->init(cb);
}


void
RenderWidget::render(QRhiCommandBuffer* cb)
{
    p->render(cb);
}

void
RenderWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        p->d.mousepos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void
RenderWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::MiddleButton) {
        QPoint delta = event->pos() - p->d.mousepos;
        p->d.pan += delta;
        p->d.mousepos = event->pos();
        update();
    }
}

void
RenderWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        unsetCursor();
    }
}

void
RenderWidget::wheelEvent(QWheelEvent* event)
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

}  // namespace flipman::sdk::widgets
