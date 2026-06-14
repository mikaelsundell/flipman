// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/renderengine.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/style.h>
#include <flipmansdk/render/shadercompiler.h>
#include <flipmansdk/render/shadercontract.h>
#include <flipmansdk/render/shaderparser.h>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QMatrix4x4>
#include <QRegularExpression>
#include <QTextStream>

#undef RENDERENGINE_STATS
#undef RENDERENGINE_TRACE

//#define RENDERENGINE_TRACE 0

#if defined(RENDERENGINE_STATS)
#    define RE_STATS_ENABLED 1
#else
#    define RE_STATS_ENABLED 0
#endif

#if defined(RENDERENGINE_TRACE)
#    define RE_TRACE_ENABLED 1
#else
#    define RE_TRACE_ENABLED 0
#endif

#if RE_TRACE_ENABLED
#    define RE_TRACE qDebug
#else
struct RenderEngineNoDebugStream {
    template<typename T> RenderEngineNoDebugStream& operator<<(const T&) { return *this; }
};
static inline RenderEngineNoDebugStream
reNoDebugStream()
{
    return {};
}
#    define RE_TRACE reNoDebugStream
#endif

namespace flipman::sdk::render {

namespace {

    const QString texUniformNv12 = QStringLiteral(R"(
layout(binding = 1) uniform sampler2D tex0;
layout(binding = 2) uniform sampler2D tex1;
)");

    const QString texUniformTexture2D = QStringLiteral(R"(
layout(binding = 1) uniform sampler2D tex;
)");

    const QString texCodeNv12 = QStringLiteral(R"(
ivec2 _sampleSize()
{
    return textureSize(tex0, 0);
}

vec4 _sample(ivec2 pixel)
{
    ivec2 ySize = textureSize(tex0, 0);
    ivec2 uvSize = textureSize(tex1, 0);
    pixel = clamp(pixel, ivec2(0), ySize - ivec2(1));
    ivec2 chromaPixel = clamp(pixel / 2, ivec2(0), uvSize - ivec2(1));
    float y = texelFetch(tex0, pixel, 0).r;
    vec2 uvv = texelFetch(tex1, chromaPixel, 0).rg;
    vec3 rgb = _nv12ToRgb(y, uvv);
    return vec4(rgb, 1.0);
}
)");

    const QString texCodeTexture2D = QStringLiteral(R"(
ivec2 _sampleSize()
{
    return textureSize(tex, 0);
}

vec4 _sample(ivec2 pixel)
{
    ivec2 size = _sampleSize();
    pixel = clamp(pixel, ivec2(0), size - ivec2(1));
    return texelFetch(tex, pixel, 0);
}
)");

    const QString texCall = QStringLiteral(R"(
    ivec2 size = _sampleSize();
    ivec2 pixel = clamp(ivec2(uv * vec2(size)), ivec2(0), size - ivec2(1));
    vec4 color = _sample(pixel);
)");

    const QString lutLookupCode = QStringLiteral(R"(
vec3 _lookup(sampler3D lut, vec3 rgb)
{
    ivec3 size = textureSize(lut, 0);
    vec3 s = vec3(size);
    vec3 v = clamp(rgb, vec3(0.0), vec3(1.0));
    vec3 uvw = (v * (s - vec3(1.0)) + vec3(0.5)) / s;
    return texture(lut, uvw).rgb;
}
)");

    const QString idtCode = QStringLiteral(R"(
vec4 idt(vec4 c)
{
    return c;
}
)");

    const QString odtCode = QStringLiteral(R"(
vec4 odt(vec4 c)
{
    return c;
}
)");

}  // namespace

class RenderEnginePrivate : public QSharedData {
public:
    RenderEnginePrivate();
    bool init(const RenderContext& context, const RenderSpec& spec);
    void reset();
    void update(QRhiResourceUpdateBatch* updates);
    void render(const RenderContext& context, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer);
    void renderScene(const RenderContext& context, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer);

public:
    struct SceneState {
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
        std::unique_ptr<QRhiRenderPassDescriptor> renderPassDescriptor;
    };

    struct QuadState {
        std::unique_ptr<QRhiBuffer> buffer;
        std::vector<float> vertices;
        QRhiVertexInputLayout layout;
        bool uploaded = false;
    };

    struct BlitState {
        QRhiRenderPassDescriptor* renderPassDescriptor = nullptr;
        std::unique_ptr<QRhiBuffer> mvpBuffer;
        std::unique_ptr<QRhiShaderResourceBindings> bindings;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        QSize size;
        bool mvpBufferUploaded = false;
    };
    bool updateBlitState(BlitState& state, QRhiRenderTarget* renderTarget, const RenderSpec& spec);
    void updateBlitTransform(BlitState& state, const RenderSpec& spec, QRhiResourceUpdateBatch* updates);
    void renderBlit(BlitState& state, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer);

    struct OutputState {
        RenderOutput* output = nullptr;
        BlitState blitState;
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
        std::unique_ptr<QRhiRenderPassDescriptor> renderPassDescriptor;
        std::unique_ptr<QRhiBuffer> convertBuffer;
        std::unique_ptr<QRhiBuffer> convertUniformBuffer;
        std::unique_ptr<QRhiShaderResourceBindings> convertBindings;
        std::unique_ptr<QRhiComputePipeline> convertPipeline;
        QRhiReadbackResult readbackResult;
        core::ImageBuffer image;
        RenderOutput::Format convertFormat = RenderOutput::Format::RGBA16F;
        RenderOutput::Format readbackFormat = RenderOutput::Format::RGBA16F;
        QSize size;
        QSize convertSize;
        QSize readbackSize;
        qsizetype readbackStride = 0;
        quint64 readbackFrameIndex = 0;
        bool readbackPending = false;
    };
    OutputState* outputState(RenderOutput* output);
    void pruneOutputStates();
    bool updateOutputState(OutputState& state, RenderOutput* output);
    void renderOutputState(OutputState& state, RenderOutput* output, QRhiCommandBuffer* commandBuffer);
    QString convertShaderName(RenderOutput::Format format) const;
    bool updateConvertState(OutputState& state, RenderOutput* output);
    void renderConvertState(OutputState& state, RenderOutput* output, QRhiCommandBuffer* commandBuffer);
    bool prepareReadbackState(OutputState& state, RenderOutput* output);
    void requestReadbackState(OutputState& state, RenderOutput* output, QRhiCommandBuffer* commandBuffer);
    
    struct LutData {
        int size = 0;
        QByteArray rgba32f;
        static QByteArray identityLut(int size)
        {
            QByteArray data;
            data.resize(size * size * size * 4 * int(sizeof(float)));
            float* dst = reinterpret_cast<float*>(data.data());
            for (int z = 0; z < size; ++z) {
                for (int y = 0; y < size; ++y) {
                    for (int x = 0; x < size; ++x) {
                        *dst++ = float(x) / float(size - 1);
                        *dst++ = float(y) / float(size - 1);
                        *dst++ = float(z) / float(size - 1);
                        *dst++ = 1.0f;
                    }
                }
            }
            return data;
        }
        static LutData loadLut(const QString& filename)
        {
            LutData data;
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "renderengine: could not open LUT:" << filename;
                return data;
            }

            int lutSize = 0;
            QVector<float> values;

            QTextStream stream(&file);
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();

                if (line.isEmpty() || line.startsWith("#"))
                    continue;

                if (line.startsWith("TITLE"))
                    continue;

                if (line.startsWith("LUT_3D_SIZE")) {
                    const QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (tokens.size() >= 2)
                        lutSize = tokens[1].toInt();
                    continue;
                }

                if (line.startsWith("DOMAIN_MIN") || line.startsWith("DOMAIN_MAX"))
                    continue;

                const QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (tokens.size() < 3)
                    continue;

                bool okR = false;
                bool okG = false;
                bool okB = false;

                const float r = tokens[0].toFloat(&okR);
                const float g = tokens[1].toFloat(&okG);
                const float b = tokens[2].toFloat(&okB);

                if (okR && okG && okB) {
                    values.append(r);
                    values.append(g);
                    values.append(b);
                }
            }

            if (lutSize < 2) {
                qWarning() << "renderengine: invalid LUT size:" << filename << lutSize;
                return data;
            }

            const int expectedTriplets = lutSize * lutSize * lutSize;
            if (values.size() != expectedTriplets * 3) {
                qWarning() << "renderengine: invalid LUT value count:" << filename << "expected" << expectedTriplets * 3
                           << "got" << values.size();
                return data;
            }

            data.size = lutSize;
            data.rgba32f.resize(expectedTriplets * 4 * int(sizeof(float)));

            float* dst = reinterpret_cast<float*>(data.rgba32f.data());
            const float* src = values.constData();

            for (int i = 0; i < expectedTriplets; ++i) {
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = 1.0f;
            }

            qDebug() << "renderengine: loaded LUT" << filename << "size" << data.size << "bytes" << data.rgba32f.size();

            return data;
        }
        bool isValid() const { return size > 1 && !rgba32f.isEmpty(); }
    };

    struct LutState {
        QString name;
        QString filename;
        int binding = -1;
        int size = 2;
        std::unique_ptr<QRhiTexture> texture;
    };

    struct ImageState {
        enum class TextureType { Unknown, UInt8, Half, Float, Nv12 };
        TextureType textureType = TextureType::Unknown;
        std::unique_ptr<QRhiTexture> texture0;
        std::unique_ptr<QRhiTexture> texture1;
        std::unique_ptr<QRhiShaderResourceBindings> shaderBindings;
        std::unique_ptr<QRhiBuffer> globalBuffer;
        std::unique_ptr<QRhiBuffer> effectBuffer;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        core::ImageBuffer imageData;
        core::ImageBuffer imageData0;
        core::ImageBuffer imageData1;
        QByteArray effectParameterData;
        std::vector<LutState> luts;
        QString lutKey;
        QString shaderKey;
        void reset()
        {
            textureType = TextureType::Unknown;
            texture0.reset();
            texture1.reset();
            shaderBindings.reset();
            globalBuffer.reset();
            effectBuffer.reset();
            pipeline.reset();
            imageData.reset();
            imageData0.reset();
            imageData1.reset();
            effectParameterData.clear();
            shaderKey.clear();
            luts.clear();
            lutKey.clear();
        }
        bool initLut(LutState& lut, const LutData& data, QRhi* rhi)
        {
            if (!rhi || !data.isValid())
                return false;

            lut.size = data.size;
            lut.texture.reset(
                rhi->newTexture(QRhiTexture::RGBA32F, lut.size, lut.size, lut.size, 1, QRhiTexture::ThreeDimensional));

            return lut.texture && lut.texture->create();
        }
        bool initTextures(const core::ImageBuffer& image, QRhi* rhi)
        {
            if (!rhi || !image.isValid())
                return false;
            if (textureType == TextureType::Nv12) {
                const QSize ySize = image.planeSize(0);
                const QSize uvSize = image.planeSize(1);
                if (!texture0 || texture0->pixelSize() != ySize || texture0->format() != QRhiTexture::R8) {
                    texture0.reset(rhi->newTexture(QRhiTexture::R8, ySize));
                    if (!texture0->create())
                        return false;
                }
                if (!texture1 || texture1->pixelSize() != uvSize || texture1->format() != QRhiTexture::RG8) {
                    texture1.reset(rhi->newTexture(QRhiTexture::RG8, uvSize));
                    if (!texture1->create())
                        return false;
                }
                return true;
            }
            const core::ImageBuffer& uploadImage = imageData0.isValid() ? imageData0 : image;
            const QSize size(uploadImage.dataWindow().width(), uploadImage.dataWindow().height());
            const QRhiTexture::Format format = toTextureFormat(textureType);

            if (!texture0 || texture0->pixelSize() != size || texture0->format() != format) {
                texture0.reset(rhi->newTexture(format, size));
                if (!texture0->create())
                    return false;
            }
            texture1.reset();
            return true;
        }
        bool prepareTextures(const core::ImageBuffer& image)
        {
            imageData0.reset();
            imageData1.reset();
            if (!image.isValid())
                return false;

            if (textureType == TextureType::Nv12) {
                imageData0 = image;
                imageData1 = image;
                return true;
            }
            if (image.channels() == 4) {
                imageData0 = image;
            }
            else {
                imageData0 = core::ImageBuffer::convert(image, image.imageFormat().type(), 4);
            }
            return imageData0.isValid();
        }
        quint64 uploadTextures(QRhiResourceUpdateBatch* updates)
        {
            if (!updates || !texture0)
                return 0;

            if (textureType == TextureType::Nv12) {
                if (!texture1 || !imageData0.isValid() || !imageData1.isValid())
                    return 0;

                QRhiTextureSubresourceUploadDescription yUpload(imageData0.planeData(0),
                                                                static_cast<quint32>(imageData0.planeByteSize(0)));
                yUpload.setDataStride(static_cast<quint32>(imageData0.planeStride(0)));
                yUpload.setSourceSize(imageData0.planeSize(0));

                updates->uploadTexture(texture0.get(),
                                       QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, yUpload) }));

                QRhiTextureSubresourceUploadDescription uvUpload(imageData1.planeData(1),
                                                                 static_cast<quint32>(imageData1.planeByteSize(1)));
                uvUpload.setDataStride(static_cast<quint32>(imageData1.planeStride(1)));
                uvUpload.setSourceSize(imageData1.planeSize(1));

                updates->uploadTexture(texture1.get(),
                                       QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, uvUpload) }));

                return quint64(imageData0.planeByteSize(0)) + quint64(imageData1.planeByteSize(1));
            }

            if (!imageData0.isValid())
                return 0;

            QRhiTextureSubresourceUploadDescription subres(imageData0.data(),
                                                           static_cast<quint32>(imageData0.byteSize()));
            subres.setDataStride(static_cast<quint32>(imageData0.strideSize()));
            subres.setSourceSize(QSize(imageData0.dataWindow().width(), imageData0.dataWindow().height()));

            updates->uploadTexture(texture0.get(),
                                   QRhiTextureUploadDescription({ QRhiTextureUploadEntry(0, 0, subres) }));

            return quint64(imageData0.byteSize());
        }
        bool uploadLut(const LutState& state, const LutData& data, QRhiResourceUpdateBatch* updates)
        {
            if (!updates || !state.texture || !data.isValid())
                return false;

            const int bytesPerPixel = 4 * int(sizeof(float));
            const int rowBytes = data.size * bytesPerPixel;
            const int sliceBytes = data.size * rowBytes;

            QVector<QRhiTextureUploadEntry> entries;
            for (int z = 0; z < state.size; ++z) {
                const char* slicePtr = data.rgba32f.constData() + z * sliceBytes;

                QRhiTextureSubresourceUploadDescription desc(slicePtr, quint32(sliceBytes));
                desc.setSourceSize(QSize(state.size, state.size));
                desc.setDataStride(quint32(rowBytes));
                entries.append(QRhiTextureUploadEntry(z, 0, desc));
            }

            QRhiTextureUploadDescription upload;
            upload.setEntries(entries.cbegin(), entries.cend());
            updates->uploadTexture(state.texture.get(), upload);
            return true;
        }
        static QRhiTexture::Format toTextureFormat(TextureType type)
        {
            switch (type) {
            case TextureType::UInt8: return QRhiTexture::RGBA8;
            case TextureType::Half: return QRhiTexture::RGBA16F;
            case TextureType::Float: return QRhiTexture::RGBA32F;
            case TextureType::Unknown:
            case TextureType::Nv12:
            default: return QRhiTexture::RGBA16F;
            }
        }
        static TextureType toTextureType(const core::ImageBuffer& image)
        {
            if (!image.isValid())
                return TextureType::Unknown;
            if (image.packing() == core::ImageBuffer::Packing::BiPlanar && image.planeCount() == 2
                && image.subsampling() == core::ImageBuffer::Subsampling::CS420) {
                return TextureType::Nv12;
            }
            switch (image.imageFormat().type()) {
            case core::ImageFormat::Type::UInt8: return TextureType::UInt8;
            case core::ImageFormat::Type::Half: return TextureType::Half;
            case core::ImageFormat::Type::Float: return TextureType::Float;
            default: return TextureType::Unknown;
            }
        }
    };

public:
    void parameterValue(char* dst, const ShaderDescriptor::ShaderParameter& param);
    QString loadShader(const QString& name);
    QShader compileShader(const QString& source, QShader::Stage stage);
    QRectF aspectFit(const QSize& src, const QSize& dst);
    int alignTo(int value, int alignment);
    int std140BaseAlignment(ShaderDescriptor::ShaderParameter::Type type);
    int std140Size(ShaderDescriptor::ShaderParameter::Type type);
    int std140BufferSize(const QList<ShaderDescriptor::ShaderParameter>& params, QVector<int>* offsets);
    qsizetype formatSize(RenderOutput::Format format, const QSize& size) const;
    qsizetype formatStride(RenderOutput::Format format, int width) const;
    QString buildLayerShaderKey(ImageState::TextureType textureType, const ShaderDefinition* effectDefinition);
    QString buildLutShaderKey(const ShaderDefinition* effectDefinition);
    QString buildLayerShaderSource(ImageState::TextureType textureType, const ShaderDefinition* effectDefinition);
    struct FileHash {
        qint64 size = -1;
        qint64 modified = -1;
        QByteArray hash;
    };
    QByteArray fileHash(const QString& filename);
    QByteArray textHash(const QString& text) const;
    
    struct FrameStats {
        quint64 frameIndex = 0;
        int layerCount = 0;
        int texturesInitialized = 0;
        int textureUploads = 0;
        quint64 textureUploadBytes = 0;
        int globalBufferUpdates = 0;
        quint64 globalBufferUploadBytes = 0;
        int effectBufferCreates = 0;
        int effectBufferUpdates = 0;
        quint64 effectBufferUploadBytes = 0;
        int shaderBindingsCreated = 0;
        int pipelinesCreated = 0;
        int shaderSourceCacheHits = 0;
        int shaderSourceCacheMisses = 0;
        int generatedShaderSourceCacheHits = 0;
        int generatedShaderSourceCacheMisses = 0;
        int shaderCacheHits = 0;
        int shaderCacheMisses = 0;
        qint64 updateRenderStatesNs = 0;
        qint64 updateBlitNs = 0;
        qint64 renderSceneNs = 0;
        qint64 renderBlitNs = 0;
        qint64 renderFrameNs = 0;
        void reset(quint64 frame, int layers)
        {
            *this = {};
            frameIndex = frame;
            layerCount = layers;
        }
    };
    void resetFrameStats();
    void logFrameStats() const;
    struct Data {
        QRhi* deviceRhi = nullptr;
        SceneState sceneState;
        QuadState quadState;
        BlitState blitState;
        std::vector<ImageState> imageStates;
        std::vector<OutputState> outputStates;
        std::unique_ptr<QRhiSampler> sampler;
        std::unique_ptr<QRhiSampler> nearestSampler;
        bool valid = false;
        QSize size;
        quint64 frameIndex = 0;
        QSize resolution = QSize(1920, 1080);
        QColor background = core::style()->color(core::Style::Viewer);
        QList<ImageLayer> imageLayers;
        QList<RenderOutput*> renderOutputs;
        QHash<QString, QString> shaderSourceCache;
        QHash<QString, QString> generatedShaderSourceCache;
        QHash<QString, QShader> shaderCache;
        QHash<QString, FileHash> fileCache;
        FrameStats stats;
        core::Error error;
    };
    Data d;
};

RenderEnginePrivate::RenderEnginePrivate() {}

bool
RenderEnginePrivate::init(const RenderContext& context, const RenderSpec& spec)
{
    if (!context.isValid()) {
        d.error = core::Error("renderengine", "invalid render context");
        return false;
    }

    d.deviceRhi = context.rhi();

    QRhiRenderTarget* renderTarget = context.surface().renderTarget();
    if (!renderTarget) {
        d.error = core::Error("renderengine", "invalid render target");
        return false;
    }

    d.size = spec.size();
    d.quadState.vertices = {
        // pos(x,y,z)      uv
        -1.f, -1.f, 0.f, 0.f, 0.f, 1.f, -1.f, 0.f, 1.f, 0.f, -1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0.f, 1.f, 1.f,
    };

    d.quadState.buffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                                    static_cast<quint32>(d.quadState.vertices.size()) * sizeof(float)));
    if (!d.quadState.buffer || !d.quadState.buffer->create()) {
        d.error = core::Error("renderengine", "could not create quad buffer");
        return false;
    }

    d.sampler.reset(d.deviceRhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                            QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    if (!d.sampler || !d.sampler->create()) {
        d.error = core::Error("renderengine", "could not create sampler");
        return false;
    }

    d.nearestSampler.reset(d.deviceRhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                   QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));

    if (!d.nearestSampler || !d.nearestSampler->create()) {
        d.error = core::Error("renderengine", "could not create nearest sampler");
        return false;
    }

    d.quadState.layout.setBindings({ { 5 * sizeof(float) } });
    d.quadState.layout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) },
    });

    d.sceneState.texture.reset(d.deviceRhi->newTexture(QRhiTexture::RGBA16F, d.resolution, 1,
                                                       QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!d.sceneState.texture || !d.sceneState.texture->create()) {
        d.error = core::Error("renderengine", "could not create render texture");
        return false;
    }

    QRhiTextureRenderTargetDescription textureRenderTargetDesc(QRhiColorAttachment(d.sceneState.texture.get()));
    d.sceneState.renderTarget.reset(d.deviceRhi->newTextureRenderTarget(textureRenderTargetDesc));
    d.sceneState.renderPassDescriptor.reset(d.sceneState.renderTarget->newCompatibleRenderPassDescriptor());
    d.sceneState.renderTarget->setRenderPassDescriptor(d.sceneState.renderPassDescriptor.get());

    if (!d.sceneState.renderTarget || !d.sceneState.renderTarget->create()) {
        d.error = core::Error("renderengine", "could not create render target");
        return false;
    }

    if (!updateBlitState(d.blitState, renderTarget, spec))
        return false;

    d.imageStates.clear();
    d.quadState.uploaded = false;
    d.valid = true;
    return true;
}

void
RenderEnginePrivate::reset()
{
    d.deviceRhi = nullptr;
    d.sceneState = {};
    d.quadState = {};
    d.blitState = {};
    d.imageStates.clear();
    d.outputStates.clear();
    d.sampler.reset();
    d.nearestSampler.reset();
    d.valid = false;
    d.frameIndex = 0;
    d.shaderSourceCache.clear();
    d.generatedShaderSourceCache.clear();
    d.shaderCache.clear();
    d.fileCache.clear();
}

void
RenderEnginePrivate::update(QRhiResourceUpdateBatch* updates)
{
    if (!updates || !d.sceneState.texture || !d.deviceRhi)
        return;

    struct Global {
        QMatrix4x4 mvp;
        QVector2D resolution;
        float time;
        float pad0;
    };

    const quint64 frameIndex = d.frameIndex;

    RE_TRACE() << "renderengine: frame: " << frameIndex;
    RE_TRACE() << "renderengine: updateRenderStates frame" << frameIndex << "layers" << d.imageLayers.size();

    if (d.imageStates.size() != size_t(d.imageLayers.size())) {
        RE_TRACE() << "renderengine: resizing render states from" << d.imageStates.size() << "to"
                   << d.imageLayers.size();
        d.imageStates.clear();
        d.imageStates.resize(size_t(d.imageLayers.size()));
    }

    const QSize targetSize = d.sceneState.texture->pixelSize();

    for (int i = 0; i < d.imageLayers.size(); ++i) {
        ImageState& imageState = d.imageStates[size_t(i)];
        const ImageLayer& imageLayer = d.imageLayers[i];
        const core::ImageBuffer image = imageLayer.image();

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "begin";

        if (!image.isValid()) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "image invalid, resetting state";
            imageState.reset();
            continue;
        }

        const ImageEffect imageEffect = imageLayer.imageEffect();
        const ShaderDefinition effectDefinition = imageEffect.shaderDefinition();

        const bool hasEffect = imageEffect.isValid() && effectDefinition.isValid()
                               && !effectDefinition.shaderCode().isEmpty();

        const ShaderDefinition* effectDefinitionPtr = hasEffect ? &effectDefinition : nullptr;

        const QSize texSize(image.dataWindow().width(), image.dataWindow().height());

        const ImageState::TextureType newType = ImageState::toTextureType(image);
        const bool typeChanged = imageState.textureType != newType;

        const bool imageLayoutChanged = typeChanged || imageState.imageData.packing() != image.packing()
                                        || imageState.imageData.subsampling() != image.subsampling()
                                        || imageState.imageData.imageFormat() != image.imageFormat()
                                        || imageState.imageData.channels() != image.channels()
                                        || imageState.imageData.dataWindow() != image.dataWindow()
                                        || imageState.imageData.displayWindow() != image.displayWindow();

        const bool imageContentChanged = !imageState.imageData.isValid() || !image.isAllocated()
                                         || !imageState.imageData.isAllocated()
                                         || imageState.imageData.data() != image.data();

        const bool imageChanged = imageLayoutChanged || imageContentChanged;

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "typeChanged" << typeChanged
                   << "imageLayoutChanged" << imageLayoutChanged << "imageContentChanged" << imageContentChanged
                   << "hasEffect" << hasEffect;

        imageState.textureType = newType;
        if (!imageState.prepareTextures(image)) {
            qWarning() << "renderengine: failed to prepare upload for image layer" << i;
            continue;
        }

        const bool needsTextures = !imageState.texture0 || imageLayoutChanged
                                   || (imageState.textureType == ImageState::TextureType::Nv12 && !imageState.texture1);

        if (needsTextures) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "initializing textures";

            if (!imageState.initTextures(image, d.deviceRhi)) {
                qWarning() << "renderengine: failed to initialize textures for layer" << i;
                continue;
            }

#if RE_STATS_ENABLED
            ++d.stats.texturesInitialized;
#endif

            imageState.shaderBindings.reset();
            imageState.pipeline.reset();
        }

        if (imageChanged || needsTextures) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "uploading textures";

            const quint64 uploadedBytes = imageState.uploadTextures(updates);

#if RE_STATS_ENABLED
            ++d.stats.textureUploads;
            d.stats.textureUploadBytes += uploadedBytes;
#endif
        }

        if (!imageState.globalBuffer) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating global buffer";

            imageState.globalBuffer.reset(
                d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(Global)));

            if (!imageState.globalBuffer->create()) {
                qWarning() << "renderengine: failed to create global buffer for layer" << i;
                imageState.globalBuffer.reset();
                continue;
            }
            imageState.shaderBindings.reset();
            imageState.pipeline.reset();
        }

        const QRectF fit = aspectFit(texSize, targetSize);
        const float sx = float(fit.width()) / float(targetSize.width());
        const float sy = float(fit.height()) / float(targetSize.height());
        const float tx = (fit.x() / float(targetSize.width())) * 2.0f;
        const float ty = (fit.y() / float(targetSize.height())) * 2.0f;

        QMatrix4x4 mvp;
        mvp.setToIdentity();
        mvp.translate(-1.0f + sx + tx, -1.0f + sy + ty);
        mvp.scale(sx, sy);

        Global global;
        global.mvp = mvp;
        global.time = 0.0f;
        global.pad0 = 0.0f;

        const QSize textureSize = imageState.texture0 ? imageState.texture0->pixelSize() : texSize;

        global.resolution = QVector2D(textureSize.width(), textureSize.height());

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "texSize" << texSize << "textureType"
                   << int(imageState.textureType) << "texture0Size"
                   << (imageState.texture0 ? imageState.texture0->pixelSize() : QSize()) << "texture1Size"
                   << (imageState.texture1 ? imageState.texture1->pixelSize() : QSize()) << "global.resolution"
                   << global.resolution;

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "updating global buffer";
        updates->updateDynamicBuffer(imageState.globalBuffer.get(), 0, sizeof(Global), &global);

#if RE_STATS_ENABLED
        ++d.stats.globalBufferUpdates;
        d.stats.globalBufferUploadBytes += sizeof(Global);
#endif

        const QString newShaderKey = buildLayerShaderKey(imageState.textureType, effectDefinitionPtr);
        const bool shaderChanged = imageState.shaderKey != newShaderKey;

        if (shaderChanged) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "shader changed";
            imageState.shaderKey = newShaderKey;
            imageState.pipeline.reset();
            imageState.effectParameterData.clear();
        }

        if (hasEffect) {
            const auto& params = effectDefinition.descriptor().uniformParameters();

            QVector<int> paramOffsets;
            const qsizetype effectBufferSize = std140BufferSize(params, &paramOffsets);
            const bool recreateEffectBuffer = !imageState.effectBuffer
                                              || imageState.effectBuffer->size() != effectBufferSize;

            if (recreateEffectBuffer) {
                RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating effect buffer size"
                           << effectBufferSize;

                imageState.effectBuffer.reset(d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                                     static_cast<quint32>(effectBufferSize)));

                if (!imageState.effectBuffer->create()) {
                    qWarning() << "renderengine: failed to create parameter buffer for layer" << i;
                    imageState.effectBuffer.reset();
                    imageState.effectParameterData.clear();
                    continue;
                }

#if RE_STATS_ENABLED
                ++d.stats.effectBufferCreates;
#endif

                imageState.effectParameterData.clear();
                imageState.shaderBindings.reset();
                imageState.pipeline.reset();
            }

            QByteArray paramData(effectBufferSize, 0);
            int uniformIndex = 0;
            for (int p = 0; p < params.size(); ++p) {
                parameterValue(paramData.data() + paramOffsets[uniformIndex], params[p]);
                ++uniformIndex;
            }

            if (imageState.effectBuffer && imageState.effectParameterData != paramData) {
                RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "updating effect buffer";

                updates->updateDynamicBuffer(imageState.effectBuffer.get(), 0, static_cast<quint32>(paramData.size()),
                                             paramData.constData());

#if RE_STATS_ENABLED
                ++d.stats.effectBufferUpdates;
                d.stats.effectBufferUploadBytes += quint64(paramData.size());
#endif

                imageState.effectParameterData = paramData;
            }
            else {
                RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "effect buffer unchanged";
            }
        }
        else if (imageState.effectBuffer) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "removing effect buffer";
            imageState.effectBuffer.reset();
            imageState.effectParameterData.clear();
            imageState.shaderBindings.reset();
            imageState.pipeline.reset();
        }

        const int lutFirstBinding = 4;
        if (effectDefinitionPtr) {
            const QList<ShaderDescriptor::ShaderParameter> params = effectDefinitionPtr->descriptor().lutParameters();

            const QString key = buildLutShaderKey(effectDefinitionPtr);

            if (imageState.lutKey != key || imageState.luts.size() != size_t(params.size())) {
                imageState.luts.clear();
                imageState.lutKey = key;

                for (int l = 0; l < params.size(); ++l) {
                    LutState lutState;
                    lutState.name = params[l].name;
                    lutState.filename = params[l].value.isValid() ? params[l].value.toString()
                                                                  : params[l].defaultValue.toString();
                    lutState.binding = lutFirstBinding + l;

                    LutData lutData = LutData::loadLut(lutState.filename);

                    if (!lutData.isValid()) {
                        qWarning() << "renderengine: failed to load LUT, using identity LUT:" << lutState.filename;
                        lutData.size = 2;
                        lutData.rgba32f = LutData::identityLut(lutData.size);
                    }

                    if (!imageState.initLut(lutState, lutData, d.deviceRhi)) {
                        qWarning() << "renderengine: failed to create LUT texture" << lutState.name << lutState.filename
                                   << "size" << lutData.size;
                        continue;
                    }

                    if (!imageState.uploadLut(lutState, lutData, updates)) {
                        qWarning() << "renderengine: failed to upload LUT texture" << lutState.name << lutState.filename
                                   << "size" << lutData.size;
                        continue;
                    }
                    imageState.luts.push_back(std::move(lutState));
                }

                imageState.shaderBindings.reset();
                imageState.pipeline.reset();
            }
        }
        else if (!imageState.luts.empty()) {
            imageState.luts.clear();
            imageState.lutKey.clear();
            imageState.shaderBindings.reset();
            imageState.pipeline.reset();
        }

        if (!imageState.shaderBindings) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating shader bindings";

            QVector<QRhiShaderResourceBinding> bindings;
            bindings << QRhiShaderResourceBinding::uniformBuffer(0,
                                                                 QRhiShaderResourceBinding::VertexStage
                                                                     | QRhiShaderResourceBinding::FragmentStage,
                                                                 imageState.globalBuffer.get());

            if (imageState.textureType == ImageState::TextureType::Nv12) {
                bindings << QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                                      imageState.texture0.get(), d.sampler.get());

                bindings << QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage,
                                                                      imageState.texture1.get(),
                                                                      d.nearestSampler.get());
            }
            else {
                bindings << QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                                      imageState.texture0.get(), d.sampler.get());
            }

            if (imageState.effectBuffer) {
                bindings << QRhiShaderResourceBinding::uniformBuffer(3, QRhiShaderResourceBinding::FragmentStage,
                                                                     imageState.effectBuffer.get());
            }

            for (const auto& lut : imageState.luts) {
                bindings << QRhiShaderResourceBinding::sampledTexture(lut.binding,
                                                                      QRhiShaderResourceBinding::FragmentStage,
                                                                      lut.texture.get(), d.sampler.get());
            }

            imageState.shaderBindings.reset(d.deviceRhi->newShaderResourceBindings());
            imageState.shaderBindings->setBindings(bindings.begin(), bindings.end());

            if (!imageState.shaderBindings->create()) {
                qWarning() << "renderengine: failed to create shader bindings for layer" << i;
                imageState.shaderBindings.reset();
                continue;
            }

#if RE_STATS_ENABLED
            ++d.stats.shaderBindingsCreated;
#endif
        }
        else {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "shader bindings reused";
        }

        if (!imageState.pipeline) {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "creating pipeline";

            imageState.pipeline.reset(d.deviceRhi->newGraphicsPipeline());
            imageState.pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

            const QString fragmentSource = buildLayerShaderSource(imageState.textureType, effectDefinitionPtr);
            if (fragmentSource.isEmpty()) {
                qWarning() << "renderengine: failed to build layer shader source:" << d.error.message();
                imageState.pipeline.reset();
                continue;
            }

            const QShader fragmentShader = compileShader(fragmentSource, QShader::FragmentStage);
            if (!fragmentShader.isValid()) {
                qWarning() << "renderengine: fragment shader compilation failed:" << d.error.message();
                imageState.pipeline.reset();
                continue;
            }

            const QString transformSource = loadShader("transform");
            if (transformSource.isEmpty()) {
                qWarning() << "renderengine: could not load transform shader";
                imageState.pipeline.reset();
                continue;
            }

            const QShader vertexShader = compileShader(transformSource, QShader::VertexStage);
            if (!vertexShader.isValid()) {
                qWarning() << "renderengine: vertex shader compilation failed:" << d.error.message();
                imageState.pipeline.reset();
                continue;
            }

            imageState.pipeline->setShaderStages(
                { { QRhiShaderStage::Vertex, vertexShader }, { QRhiShaderStage::Fragment, fragmentShader } });

            imageState.pipeline->setVertexInputLayout(d.quadState.layout);

            QRhiGraphicsPipeline::TargetBlend blend;
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

            imageState.pipeline->setTargetBlends({ blend });
            imageState.pipeline->setShaderResourceBindings(imageState.shaderBindings.get());
            imageState.pipeline->setRenderPassDescriptor(d.sceneState.renderPassDescriptor.get());

            if (!imageState.pipeline->create()) {
                qWarning() << "renderengine: failed to create pipeline for layer" << i;
                imageState.pipeline.reset();
                continue;
            }

#if RE_STATS_ENABLED
            ++d.stats.pipelinesCreated;
#endif
        }
        else {
            RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "pipeline reused";
        }

        imageState.imageData = image;

        RE_TRACE() << "renderengine: frame" << frameIndex << "layer" << i << "done";
    }
}

void
RenderEnginePrivate::render(const RenderContext& context, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer)
{
#if RE_STATS_ENABLED
    QElapsedTimer frameTimer;
    QElapsedTimer timer;
    frameTimer.start();
#endif

    ++d.frameIndex;
    resetFrameStats();

    // upload static fullscreen quad data once.
    if (!d.quadState.uploaded) {
        QRhiResourceUpdateBatch* u = d.deviceRhi->nextResourceUpdateBatch();
        u->updateDynamicBuffer(d.quadState.buffer.get(), 0,
                               static_cast<quint32>(d.quadState.vertices.size() * sizeof(float)),
                               d.quadState.vertices.data());
        commandBuffer->resourceUpdate(u);
        d.quadState.uploaded = true;
    }

    QRhiResourceUpdateBatch* resourceUpdates = d.deviceRhi->nextResourceUpdateBatch();

#if RE_STATS_ENABLED
    timer.start();
#endif

    // prepare layer textures, shader resources, uniforms, LUTs and pipelines.
    // This updates all resources needed by the scene pass.
    update(resourceUpdates);

#if RE_STATS_ENABLED
    d.stats.updateRenderStatesNs = timer.nsecsElapsed();
    timer.restart();
#endif

    // prepare the primary output transform. The primary output is the target
    // supplied by the caller through RenderSpec.
    updateBlitTransform(d.blitState, spec, resourceUpdates);

    // prepare additional output targets and transforms.
    // Each output owns an RGBA16F render target. For now this only prepares the
    // output blit target; format conversion/readback will be added after this.
    pruneOutputStates();

    for (RenderOutput* output : d.renderOutputs) {
        if (!output || !output->enabled())
            continue;

        OutputState* state = outputState(output);
        if (!state)
            continue;

        if (!updateOutputState(*state, output))
            continue;

        updateBlitTransform(state->blitState, output->pass(), resourceUpdates);
    }

#if RE_STATS_ENABLED
    d.stats.updateBlitNs = timer.nsecsElapsed();
    timer.restart();
#endif

    // scene pass
    // render all image layers into the engine-owned internal RGBA16F target.
    commandBuffer->beginPass(d.sceneState.renderTarget.get(), d.background, { 1.0f, 0 }, resourceUpdates);
    renderScene(context, spec, commandBuffer);
    commandBuffer->endPass();

#if RE_STATS_ENABLED
    d.stats.renderSceneNs = timer.nsecsElapsed();
    timer.restart();
#endif

    // blit pass
    // sample the internal RGBA16F target and draw it into the caller-provided
    // render target, applying the primary RenderSpec view transform.
    commandBuffer->beginPass(context.surface().renderTarget(), d.background, { 1.0f, 0 });
    renderBlit(d.blitState, spec, commandBuffer);
    commandBuffer->endPass();

    // additional output passes
    // sample the same internal RGBA16F target and draw it into each output-owned
    // RGBA16F render target, applying the output RenderSpec view transform.
    
    RE_TRACE() << "renderengine: outputs"
               << "configured" << d.renderOutputs.size()
               << "states" << d.outputStates.size();
    
    for (RenderOutput* output : d.renderOutputs) {
        if (!output || !output->enabled())
            continue;

        OutputState* state = outputState(output);
        if (!state)
            continue;

        renderOutputState(*state, output, commandBuffer);
        
        if (updateConvertState(*state, output)) {
            renderConvertState(*state, output, commandBuffer);
            if (prepareReadbackState(*state, output))
                requestReadbackState(*state, output, commandBuffer);
        }
    }

#if RE_STATS_ENABLED
    d.stats.renderBlitNs = timer.nsecsElapsed();
    d.stats.renderFrameNs = frameTimer.nsecsElapsed();
    logFrameStats();

    qDebug() << "\n";
#endif
}

void
RenderEnginePrivate::renderScene(const RenderContext& context, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer)
{
    if (!d.quadState.buffer)
        return;

    const QSize size = spec.size();
    const QSize targetSize = d.sceneState.texture->pixelSize();
    commandBuffer->setViewport({ 0, 0, float(targetSize.width()), float(targetSize.height()) });

    for (int i = 0; i < d.imageLayers.size(); ++i) {
        if (size_t(i) >= d.imageStates.size())
            continue;

        ImageState& imageState = d.imageStates[size_t(i)];
        if (!imageState.pipeline.get()) {
            continue;
        }

        if (!imageState.shaderBindings)
            continue;

        commandBuffer->setGraphicsPipeline(imageState.pipeline.get());
        commandBuffer->setShaderResources(imageState.shaderBindings.get());

        const QRhiCommandBuffer::VertexInput vbufBinding(d.quadState.buffer.get(), 0);
        commandBuffer->setVertexInput(0, 1, &vbufBinding);
        commandBuffer->draw(4);
    }
}

bool
RenderEnginePrivate::updateBlitState(BlitState& state, QRhiRenderTarget* renderTarget, const RenderSpec& spec)
{
    if (!d.deviceRhi || !d.sampler || !d.sceneState.texture || !renderTarget) {
        d.error = core::Error("renderengine", "invalid blit state");
        return false;
    }

    QRhiRenderPassDescriptor* renderPassDescriptor = renderTarget->renderPassDescriptor();
    if (!renderPassDescriptor) {
        d.error = core::Error("renderengine", "invalid blit render pass descriptor");
        return false;
    }

    const QSize size = spec.size();
    const bool stateChanged = state.renderPassDescriptor != renderPassDescriptor || state.size != size;

    state.renderPassDescriptor = renderPassDescriptor;
    state.size = size;

    if (!state.mvpBuffer) {
        state.mvpBuffer.reset(
            d.deviceRhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(float) * 16));

        if (!state.mvpBuffer || !state.mvpBuffer->create()) {
            d.error = core::Error("renderengine", "could not create blit MVP buffer");
            state.mvpBuffer.reset();
            return false;
        }

        state.mvpBufferUploaded = false;
        state.bindings.reset();
        state.pipeline.reset();
    }

    if (stateChanged) {
        state.pipeline.reset();
    }

    if (!state.bindings) {
        state.bindings.reset(d.deviceRhi->newShaderResourceBindings());
        state.bindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0,
                                                     QRhiShaderResourceBinding::VertexStage
                                                         | QRhiShaderResourceBinding::FragmentStage,
                                                     state.mvpBuffer.get()),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      d.sceneState.texture.get(), d.sampler.get()),
        });

        if (!state.bindings->create()) {
            d.error = core::Error("renderengine", "could not create blit shader bindings");
            state.bindings.reset();
            return false;
        }
    }

    if (!state.pipeline) {
        state.pipeline.reset(d.deviceRhi->newGraphicsPipeline());
        state.pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);

        const QShader vertexShader = compileShader(loadShader("transform"), QShader::VertexStage);
        if (!vertexShader.isValid()) {
            d.error = core::Error("renderengine", "could not compile transform shader");
            state.pipeline.reset();
            return false;
        }

        const QShader fragmentShader = compileShader(loadShader("blit"), QShader::FragmentStage);
        if (!fragmentShader.isValid()) {
            d.error = core::Error("renderengine", "could not compile blit shader");
            state.pipeline.reset();
            return false;
        }

        state.pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, vertexShader },
            { QRhiShaderStage::Fragment, fragmentShader },
        });

        state.pipeline->setVertexInputLayout(d.quadState.layout);
        state.pipeline->setShaderResourceBindings(state.bindings.get());
        state.pipeline->setRenderPassDescriptor(state.renderPassDescriptor);

        if (!state.pipeline->create()) {
            d.error = core::Error("renderengine", "could not create blit pipeline");
            state.pipeline.reset();
            return false;
        }
    }

    return true;
}

void
RenderEnginePrivate::updateBlitTransform(BlitState& state, const RenderSpec& spec, QRhiResourceUpdateBatch* updates)
{
    if (!updates || !d.sceneState.texture || !state.mvpBuffer)
        return;

    const QSize targetSize = spec.size();
    const QSize textureSize = d.sceneState.texture->pixelSize();

    if (targetSize.isEmpty() || textureSize.isEmpty())
        return;

    const float sx = float(textureSize.width()) / float(targetSize.width());
    const float sy = float(textureSize.height()) / float(targetSize.height());

    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.scale(sx, sy);

    const QMatrix4x4 finalMatrix = spec.view() * matrix;

    updates->updateDynamicBuffer(state.mvpBuffer.get(), 0, 64, finalMatrix.constData());
}

void
RenderEnginePrivate::renderBlit(BlitState& state, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer)
{
    if (!state.pipeline || !state.bindings || !d.quadState.buffer)
        return;

    commandBuffer->setGraphicsPipeline(state.pipeline.get());
    commandBuffer->setShaderResources(state.bindings.get());

    const QSize size = spec.size();
    commandBuffer->setViewport({ 0, 0, float(size.width()), float(size.height()) });

    const QRhiCommandBuffer::VertexInput vbufBinding(d.quadState.buffer.get(), 0);
    commandBuffer->setVertexInput(0, 1, &vbufBinding);
    commandBuffer->draw(4);
}

RenderEnginePrivate::OutputState*
RenderEnginePrivate::outputState(RenderOutput* output)
{
    if (!output)
        return nullptr;

    for (OutputState& state : d.outputStates) {
        if (state.output == output) {
            RE_TRACE() << "renderengine: output state reused"
                       << "output" << output
                       << "size" << state.size;
            return &state;
        }
    }

    OutputState state;
    state.output = output;
    d.outputStates.push_back(std::move(state));

    RE_TRACE() << "renderengine: output state created"
               << "output" << output
               << "stateCount" << d.outputStates.size();

    return &d.outputStates.back();
}

void
RenderEnginePrivate::pruneOutputStates()
{
    auto it = d.outputStates.begin();
    while (it != d.outputStates.end()) {
        if (!it->output || !d.renderOutputs.contains(it->output)) {
            RE_TRACE() << "renderengine: output state pruned"
                       << "output" << it->output
                       << "size" << it->size;

            it = d.outputStates.erase(it);
        }
        else {
            ++it;
        }
    }
}

bool
RenderEnginePrivate::updateOutputState(OutputState& state, RenderOutput* output)
{
    if (!d.deviceRhi || !output) {
        RE_TRACE() << "renderengine: output update skipped invalid output";
        return false;
    }

    const RenderSpec outputSpec = output->pass();
    if (!outputSpec.isValid()) {
        RE_TRACE() << "renderengine: output update skipped invalid pass"
                   << "output" << output;
        return false;
    }

    const QSize size = outputSpec.size();
    if (size.isEmpty()) {
        RE_TRACE() << "renderengine: output update skipped empty size"
                   << "output" << output
                   << "size" << size;
        return false;
    }

    const bool recreateTarget =
        !state.texture ||
        !state.renderTarget ||
        state.size != size ||
        state.texture->pixelSize() != size ||
        state.texture->format() != QRhiTexture::RGBA16F;

    RE_TRACE() << "renderengine: output update"
               << "output" << output
               << "size" << size
               << "currentSize" << state.size
               << "recreateTarget" << recreateTarget;

    if (recreateTarget) {
        RE_TRACE() << "renderengine: output creating RGBA16F target"
                   << "output" << output
                   << "size" << size;

        state.texture.reset(d.deviceRhi->newTexture(
            QRhiTexture::RGBA16F,
            size,
            1,
            QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));

        if (!state.texture || !state.texture->create()) {
            d.error = core::Error("renderengine", "could not create output render texture");

            RE_TRACE() << "renderengine: output texture create failed"
                       << "output" << output
                       << "size" << size;

            state = {};
            state.output = output;
            return false;
        }

        QRhiTextureRenderTargetDescription desc(QRhiColorAttachment(state.texture.get()));

        state.renderTarget.reset(d.deviceRhi->newTextureRenderTarget(desc));
        state.renderPassDescriptor.reset(state.renderTarget->newCompatibleRenderPassDescriptor());
        state.renderTarget->setRenderPassDescriptor(state.renderPassDescriptor.get());

        if (!state.renderTarget || !state.renderTarget->create()) {
            d.error = core::Error("renderengine", "could not create output render target");

            RE_TRACE() << "renderengine: output render target create failed"
                       << "output" << output
                       << "size" << size;

            state = {};
            state.output = output;
            return false;
        }

        state.size = size;

        state.blitState.renderPassDescriptor = nullptr;
        state.blitState.pipeline.reset();
        state.blitState.bindings.reset();

        RE_TRACE() << "renderengine: output target created"
                   << "output" << output
                   << "texture" << state.texture.get()
                   << "renderTarget" << state.renderTarget.get()
                   << "renderPassDescriptor" << state.renderPassDescriptor.get()
                   << "size" << state.size;
    }

    const bool ok = updateBlitState(state.blitState, state.renderTarget.get(), outputSpec);

    RE_TRACE() << "renderengine: output blit state"
               << "output" << output
               << "ok" << ok
               << "pipeline" << state.blitState.pipeline.get()
               << "bindings" << state.blitState.bindings.get();

    return ok;
}

void
RenderEnginePrivate::renderOutputState(OutputState& state, RenderOutput* output, QRhiCommandBuffer* commandBuffer)
{
    if (!output || !output->enabled() || !commandBuffer) {
        RE_TRACE() << "renderengine: output render skipped invalid output/commandBuffer"
                   << "output" << output;
        return;
    }

    const RenderSpec outputSpec = output->pass();
    if (!outputSpec.isValid()) {
        RE_TRACE() << "renderengine: output render skipped invalid pass"
                   << "output" << output;
        return;
    }

    if (!state.renderTarget || !state.texture) {
        RE_TRACE() << "renderengine: output render skipped missing target"
                   << "output" << output
                   << "texture" << state.texture.get()
                   << "renderTarget" << state.renderTarget.get();
        return;
    }

    RE_TRACE() << "renderengine: output render begin"
               << "output" << output
               << "size" << outputSpec.size()
               << "texture" << state.texture.get()
               << "renderTarget" << state.renderTarget.get();

    commandBuffer->beginPass(state.renderTarget.get(), Qt::transparent, { 1.0f, 0 });
    renderBlit(state.blitState, outputSpec, commandBuffer);
    commandBuffer->endPass();

    RE_TRACE() << "renderengine: output render end"
               << "output" << output;
}

QString
RenderEnginePrivate::convertShaderName(RenderOutput::Format format) const
{
    switch (format) {
    case RenderOutput::Format::RGBA8: return QStringLiteral("rgba8");
    case RenderOutput::Format::UYVY8: return QStringLiteral("cuyvy8");
    case RenderOutput::Format::V210: return QStringLiteral("v210");
    case RenderOutput::Format::RGBA16F: break;
    }

    return {};
}

bool
RenderEnginePrivate::updateConvertState(OutputState& state, RenderOutput* output)
{
    if (!d.deviceRhi || !d.sampler || !output || !state.texture)
        return false;

    const RenderSpec outputSpec = output->pass();
    if (!outputSpec.isValid())
        return false;

    const QSize size = outputSpec.size();
    const RenderOutput::Format format = output->format();

    if (format == RenderOutput::Format::RGBA16F)
        return true;

    const qsizetype byteSize = formatSize(format, size);
    if (byteSize <= 0)
        return false;

    const bool recreate =
        !state.convertBuffer ||
        state.convertSize != size ||
        state.convertFormat != format ||
        state.convertBuffer->size() != quint32(byteSize);

    if (recreate) {
        state.convertBuffer.reset(d.deviceRhi->newBuffer(
            QRhiBuffer::Static,
            QRhiBuffer::StorageBuffer,
            quint32(byteSize)));

        if (!state.convertBuffer || !state.convertBuffer->create()) {
            d.error = core::Error("renderengine", "could not create output convert buffer");
            state.convertBuffer.reset();
            return false;
        }

        state.convertUniformBuffer.reset(d.deviceRhi->newBuffer(
            QRhiBuffer::Dynamic,
            QRhiBuffer::UniformBuffer,
            16));

        if (!state.convertUniformBuffer || !state.convertUniformBuffer->create()) {
            d.error = core::Error("renderengine", "could not create output convert uniform buffer");
            state.convertUniformBuffer.reset();
            return false;
        }

        state.convertSize = size;
        state.convertFormat = format;
        state.convertBindings.reset();
        state.convertPipeline.reset();

        RE_TRACE() << "renderengine: output convert buffer created"
                   << "output" << output
                   << "format" << int(format)
                   << "size" << size
                   << "bytes" << byteSize;
    }

    if (!state.convertBindings) {
        state.convertBindings.reset(d.deviceRhi->newShaderResourceBindings());
        state.convertBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(
                0,
                QRhiShaderResourceBinding::ComputeStage,
                state.convertUniformBuffer.get()),

            QRhiShaderResourceBinding::sampledTexture(
                1,
                QRhiShaderResourceBinding::ComputeStage,
                state.texture.get(),
                d.sampler.get()),

            QRhiShaderResourceBinding::bufferLoadStore(
                2,
                QRhiShaderResourceBinding::ComputeStage,
                state.convertBuffer.get()),
        });

        if (!state.convertBindings->create()) {
            d.error = core::Error("renderengine", "could not create output convert bindings");
            state.convertBindings.reset();
            return false;
        }
    }

    if (!state.convertPipeline) {
        const QString shaderName = convertShaderName(format);
        if (shaderName.isEmpty())
            return false;

        const QShader shader = compileShader(loadShader(shaderName), QShader::ComputeStage);
        if (!shader.isValid()) {
            d.error = core::Error("renderengine", "could not compile output convert shader");
            state.convertPipeline.reset();
            return false;
        }

        state.convertPipeline.reset(d.deviceRhi->newComputePipeline());
        state.convertPipeline->setShaderStage({ QRhiShaderStage::Compute, shader });
        state.convertPipeline->setShaderResourceBindings(state.convertBindings.get());

        if (!state.convertPipeline->create()) {
            d.error = core::Error("renderengine", "could not create output convert pipeline");
            state.convertPipeline.reset();
            return false;
        }

        RE_TRACE() << "renderengine: output convert pipeline created"
                   << "output" << output
                   << "format" << int(format)
                   << "shader" << shaderName;
    }

    return true;
}

void
RenderEnginePrivate::renderConvertState(OutputState& state, RenderOutput* output, QRhiCommandBuffer* commandBuffer)
{
    if (!output || !output->enabled() || !commandBuffer)
        return;

    const RenderOutput::Format format = output->format();
    if (format == RenderOutput::Format::RGBA16F)
        return;

    if (!state.convertPipeline || !state.convertBindings || !state.convertUniformBuffer)
        return;

    const RenderSpec outputSpec = output->pass();
    const QSize size = outputSpec.size();
    const int stride = int(formatStride(format, size.width()));

    struct ConvertUniforms {
        QVector2D size;
        quint32 stride;
        quint32 pad0;
    };

    ConvertUniforms uniforms;
    uniforms.size = QVector2D(size.width(), size.height());
    uniforms.stride = quint32(stride);
    uniforms.pad0 = 0;

    QRhiResourceUpdateBatch* updates = d.deviceRhi->nextResourceUpdateBatch();
    updates->updateDynamicBuffer(state.convertUniformBuffer.get(), 0, sizeof(ConvertUniforms), &uniforms);
    commandBuffer->resourceUpdate(updates);

    const int groupSize = 16;
    const int groupsX = (size.width() + groupSize - 1) / groupSize;
    const int groupsY = (size.height() + groupSize - 1) / groupSize;

    RE_TRACE() << "renderengine: output convert dispatch"
               << "output" << output
               << "format" << int(format)
               << "size" << size
               << "stride" << stride
               << "groups" << QSize(groupsX, groupsY);

    commandBuffer->beginComputePass();
    commandBuffer->setComputePipeline(state.convertPipeline.get());
    commandBuffer->setShaderResources(state.convertBindings.get());
    commandBuffer->dispatch(groupsX, groupsY, 1);
    commandBuffer->endComputePass();
}

bool
RenderEnginePrivate::prepareReadbackState(OutputState& state, RenderOutput* output)
{
    if (!output)
        return false;

    const RenderSpec outputSpec = output->pass();
    if (!outputSpec.isValid())
        return false;

    const QSize size = outputSpec.size();
    if (size.isEmpty())
        return false;

    const RenderOutput::Format format = output->format();
    const qsizetype stride = formatStride(format, size.width());
    const qsizetype byteSize = formatSize(format, size);

    if (stride <= 0 || byteSize <= 0)
        return false;

    const bool changed =
        !state.image.isValid() ||
        !state.image.isAllocated() ||
        state.readbackFormat != format ||
        state.readbackSize != size ||
        state.readbackStride != stride ||
        qsizetype(state.image.byteSize()) != byteSize;

    if (!changed)
        return true;

    state.image.reset();

    const QRect displayWindow(0, 0, size.width(), size.height());

    switch (format) {
    case RenderOutput::Format::RGBA16F: {
        state.image = core::ImageBuffer(
            displayWindow,
            displayWindow,
            core::ImageFormat(core::ImageFormat::Type::Half),
            4);

        state.image.setPacking(core::ImageBuffer::Packing::Interleaved);
        state.image.setSubsampling(core::ImageBuffer::Subsampling::None);
        break;
    }

    case RenderOutput::Format::RGBA8: {
        state.image = core::ImageBuffer(
            displayWindow,
            displayWindow,
            core::ImageFormat(core::ImageFormat::Type::UInt8),
            4);

        state.image.setPacking(core::ImageBuffer::Packing::Interleaved);
        state.image.setSubsampling(core::ImageBuffer::Subsampling::None);
        break;
    }

    case RenderOutput::Format::UYVY8:
    case RenderOutput::Format::V210: {
        const QRect dataWindow(0, 0, int(stride), size.height());

        state.image = core::ImageBuffer(
            dataWindow,
            displayWindow,
            core::ImageFormat(core::ImageFormat::Type::UInt8),
            1);

        state.image.setPacking(core::ImageBuffer::Packing::Packed);
        state.image.setSubsampling(core::ImageBuffer::Subsampling::CS422);
        break;
    }
    }

    state.image.allocate();

    if (!state.image.isValid() || !state.image.isAllocated()) {
        RE_TRACE() << "renderengine: readback image create failed"
                   << "output" << output
                   << "format" << int(format)
                   << "size" << size
                   << "stride" << stride
                   << "bytes" << byteSize;
        state.image.reset();
        return false;
    }

    state.readbackFormat = format;
    state.readbackSize = size;
    state.readbackStride = stride;

    RE_TRACE() << "renderengine: readback image prepared"
               << "output" << output
               << "format" << int(format)
               << "dataWindow" << state.image.dataWindow()
               << "displayWindow" << state.image.displayWindow()
               << "stride" << state.image.strideSize()
               << "bytes" << state.image.byteSize();

    return true;
}

void
RenderEnginePrivate::requestReadbackState(OutputState& state, RenderOutput* output, QRhiCommandBuffer* commandBuffer)
{
    if (!output || !commandBuffer || state.readbackPending)
        return;

    if (!state.convertBuffer || !state.image.isValid() || !state.image.isAllocated())
        return;

    const qsizetype byteSize = qsizetype(state.image.byteSize());
    if (byteSize <= 0 || byteSize > std::numeric_limits<quint32>::max())
        return;

    state.readbackPending = true;
    state.readbackFrameIndex = d.frameIndex;
    state.readbackResult = {};

    state.readbackResult.completed = [&state, output]() {
        const qsizetype srcSize = state.readbackResult.data.size();
        const qsizetype dstSize = qsizetype(state.image.byteSize());

        RE_TRACE() << "renderengine: readback complete"
                   << "output" << output
                   << "frame" << state.readbackFrameIndex
                   << "srcBytes" << srcSize
                   << "dstBytes" << dstSize;

        if (srcSize >= dstSize) {
            memcpy(state.image.data(),
                   state.readbackResult.data.constData(),
                   size_t(dstSize));

            output->enqueueFrame(state.image, state.readbackFrameIndex);
        }

        state.readbackPending = false;
    };

    QRhiResourceUpdateBatch* updates = d.deviceRhi->nextResourceUpdateBatch();
    updates->readBackBuffer(
        state.convertBuffer.get(),
        0,
        quint32(byteSize),
        &state.readbackResult);

    commandBuffer->resourceUpdate(updates);

    RE_TRACE() << "renderengine: readback requested"
               << "output" << output
               << "frame" << state.readbackFrameIndex
               << "bytes" << byteSize;
}

void
RenderEnginePrivate::parameterValue(char* dst, const ShaderDescriptor::ShaderParameter& param)
{
    const QVariant value = param.value.isValid() ? param.value : param.defaultValue;

    switch (param.type) {
    case ShaderDescriptor::ShaderParameter::Type::Float: {
        const float v = value.toFloat();
        memcpy(dst, &v, sizeof(float));
        break;
    }
    case ShaderDescriptor::ShaderParameter::Type::Int: {
        const int v = value.toInt();
        memcpy(dst, &v, sizeof(int));
        break;
    }
    case ShaderDescriptor::ShaderParameter::Type::Bool: {
        const int v = value.toBool() ? 1 : 0;
        memcpy(dst, &v, sizeof(int));
        break;
    }
    case ShaderDescriptor::ShaderParameter::Type::Vec2: {
        const QVector2D v = value.value<QVector2D>();
        const float data[2] = { v.x(), v.y() };
        memcpy(dst, data, sizeof(data));
        break;
    }
    case ShaderDescriptor::ShaderParameter::Type::Vec3: {
        const QVector3D v = value.value<QVector3D>();
        const float data[3] = { v.x(), v.y(), v.z() };
        memcpy(dst, data, sizeof(data));
        break;
    }
    case ShaderDescriptor::ShaderParameter::Type::Vec4: {
        const QVector4D v = value.value<QVector4D>();
        const float data[4] = { v.x(), v.y(), v.z(), v.w() };
        memcpy(dst, data, sizeof(data));
        break;
    }
    case ShaderDescriptor::ShaderParameter::Type::Lut: break;
    }
}

QString
RenderEnginePrivate::loadShader(const QString& name)
{
    const auto it = d.shaderSourceCache.constFind(name);
    if (it != d.shaderSourceCache.constEnd()) {
#if RE_STATS_ENABLED
        ++d.stats.shaderSourceCacheHits;
#endif
        RE_TRACE() << "renderengine: shader source cache hit:" << name;
        return it.value();
    }

#if RE_STATS_ENABLED
    ++d.stats.shaderSourceCacheMisses;
#endif

    QFile file(":/flipmansdk/glsl/" + name + ".glsl");
    if (!file.open(QIODevice::ReadOnly)) {
        d.error = core::Error("renderengine", "could not load shader for: " + name);
        return QString();
    }

    const QByteArray bytes = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString source = decoder.decode(bytes);

    if (decoder.hasError()) {
        d.error = core::Error("shaderinterpreter", "shader file is not valid utf-8: " + name);
        return {};
    }

    file.close();

    d.shaderSourceCache.insert(name, source);
    RE_TRACE() << "renderengine: shader source cache miss:" << name;

    return source;
}

QShader
RenderEnginePrivate::compileShader(const QString& source, QShader::Stage stage)
{
    if (!d.deviceRhi)
        return QShader();

    const QRhi::Implementation impl = d.deviceRhi->backend();
    const QByteArray hash = QCryptographicHash::hash(source.toUtf8(), QCryptographicHash::Sha1).toHex();
    const QString key = QString("%1:%2:%3").arg(int(impl)).arg(int(stage)).arg(QString::fromLatin1(hash));

#if RE_TRACE_ENABLED
    const QString stageName = stage == QShader::VertexStage     ? "vertex"
                              : stage == QShader::FragmentStage ? "fragment"
                                                                : "shader";
    qDebug().noquote() << "stage:" << stageName << "\n"
                       << "key:" << key << "\n"
                       << source << "\n";
#endif

    const auto it = d.shaderCache.constFind(key);
    if (it != d.shaderCache.constEnd()) {
#if RE_STATS_ENABLED
        ++d.stats.shaderCacheHits;
#endif
        RE_TRACE() << "renderengine: shader cache hit:" << key;
        return it.value();
    }

#if RE_STATS_ENABLED
    ++d.stats.shaderCacheMisses;
#endif
    RE_TRACE() << "renderengine: shader cache miss:" << key;

    render::ShaderCompiler compiler;
    render::ShaderCompiler::Options opts;
    opts.glslVersion = 440;

    switch (impl) {
    case QRhi::Vulkan: opts.generateSpirv = true; break;

    case QRhi::Metal:
        opts.generateSpirv = true;
        opts.generateMsl = true;
        break;

    case QRhi::D3D11:
    case QRhi::D3D12:
        opts.generateSpirv = true;
        opts.generateHlsl = true;
        break;

    case QRhi::OpenGLES2: opts.generateSpirv = true; break;

    default:
        opts.generateSpirv = true;
        opts.generateMsl = true;
        opts.generateHlsl = true;
        break;
    }

    QShader shader = compiler.compile(source, stage, opts);
    if (!shader.isValid()) {
        d.error = compiler.error();
        return QShader();
    }

    d.shaderCache.insert(key, shader);
    return shader;
}

QRectF
RenderEnginePrivate::aspectFit(const QSize& src, const QSize& dst)
{
    if (src.isEmpty() || dst.isEmpty())
        return {};
    const float srcAspect = float(src.width()) / float(src.height());
    const float dstAspect = float(dst.width()) / float(dst.height());
    float w, h;
    if (dstAspect > srcAspect) {
        h = float(dst.height());
        w = h * srcAspect;
    }
    else {
        w = float(dst.width());
        h = w / srcAspect;
    }
    const float x = (dst.width() - w) * 0.5f;
    const float y = (dst.height() - h) * 0.5f;
    return QRectF(x, y, w, h);
}

int
RenderEnginePrivate::alignTo(int value, int alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

int
RenderEnginePrivate::std140BaseAlignment(ShaderDescriptor::ShaderParameter::Type type)
{
    using Type = ShaderDescriptor::ShaderParameter::Type;
    switch (type) {
    case Type::Float:
    case Type::Int:
    case Type::Bool: return 4;
    case Type::Vec2: return 8;
    case Type::Vec3:
    case Type::Vec4: return 16;
    case Type::Lut: return 0;
    }
    return 16;
}

int
RenderEnginePrivate::std140Size(ShaderDescriptor::ShaderParameter::Type type)
{
    using Type = ShaderDescriptor::ShaderParameter::Type;
    switch (type) {
    case Type::Float:
    case Type::Int:
    case Type::Bool: return 4;
    case Type::Vec2: return 8;
    case Type::Vec3: return 12;
    case Type::Vec4: return 16;
    case Type::Lut: return 0;
    }
    return 16;
}

int
RenderEnginePrivate::std140BufferSize(const QList<ShaderDescriptor::ShaderParameter>& params, QVector<int>* offsets)
{
    offsets->clear();
    int offset = 0;
    for (const auto& param : params) {
        const int alignment = std140BaseAlignment(param.type);
        offset = alignTo(offset, alignment);
        offsets->push_back(offset);
        // Important:
        // vec3 has 16-byte base alignment, but its occupied value size is 12 bytes.
        // A following float/int/bool may legally occupy the remaining 4 bytes.
        offset += std140Size(param.type);
    }
    return alignTo(offset, 16);
}

qsizetype
RenderEnginePrivate::formatStride(RenderOutput::Format format, int width) const
{
    if (width <= 0)
        return 0;

    switch (format) {
    case RenderOutput::Format::RGBA16F:
        return qsizetype(width) * 4 * qsizetype(sizeof(quint16));

    case RenderOutput::Format::RGBA8:
        return qsizetype(width) * 4;

    case RenderOutput::Format::UYVY8:
        // 4:2:2 packed: 2 pixels = 4 bytes.
        return qsizetype(width) * 2;

    case RenderOutput::Format::V210:
        // v210 rows are padded to 48-pixel / 128-byte alignment.
        return qsizetype((width + 47) / 48) * 128;
    }

    return 0;
}

qsizetype
RenderEnginePrivate::formatSize(RenderOutput::Format format, const QSize& size) const
{
    if (size.isEmpty())
        return 0;

    return formatStride(format, size.width()) * qsizetype(size.height());
}

QString
RenderEnginePrivate::buildLayerShaderKey(ImageState::TextureType textureType, const ShaderDefinition* effectDefinition)
{
    QString effectShaderCode;
    QString effectUniformBlock;

    if (effectDefinition) {
        effectShaderCode = effectDefinition->shaderCode();
        effectUniformBlock = effectDefinition->uniformBlock(3);
    }

    return QString("layer:%1:%2:%3:%4:%5:%6")
        .arg(int(textureType))
        .arg(QString::fromLatin1(textHash(idtCode)))
        .arg(QString::fromLatin1(textHash(odtCode)))
        .arg(QString::fromLatin1(textHash(effectShaderCode)))
        .arg(QString::fromLatin1(textHash(effectUniformBlock)))
        .arg(QString::fromLatin1(textHash(buildLutShaderKey(effectDefinition))));
}

QString
RenderEnginePrivate::buildLayerShaderSource(ImageState::TextureType textureType,
                                            const ShaderDefinition* effectDefinition)
{
    const QString shaderKey = buildLayerShaderKey(textureType, effectDefinition);
    const auto it = d.generatedShaderSourceCache.constFind(shaderKey);
    if (it != d.generatedShaderSourceCache.constEnd()) {
#if RE_STATS_ENABLED
        ++d.stats.generatedShaderSourceCacheHits;
#endif
        RE_TRACE() << "renderengine: generated shader source cache hit:" << shaderKey;
        return it.value();
    }

#if RE_STATS_ENABLED
    ++d.stats.generatedShaderSourceCacheMisses;
#endif
    RE_TRACE() << "renderengine: generated shader source cache miss:" << shaderKey;

    const QString layerSource = loadShader("layer");
    if (layerSource.isEmpty())
        return {};

    ShaderParser shaderParser;
    ShaderContract shaderContract;

    const ShaderDefinition idtDefinition = shaderParser.parse(idtCode);
    if (!shaderParser.isValid()) {
        d.error = core::Error("renderengine", "failed to parse idt shader: " + shaderParser.error().message());
        return {};
    }

    const ShaderContract::Type idtType = ShaderContract::Type::Idt;
    if (!shaderContract.validate(idtType, idtDefinition.functions())) {
        d.error = core::Error("renderengine", "idt shader does not satisfy " + shaderContract.name(idtType));
        return {};
    }

    const ShaderDefinition odtDefinition = shaderParser.parse(odtCode);
    if (!shaderParser.isValid()) {
        d.error = core::Error("renderengine", "failed to parse odt shader: " + shaderParser.error().message());
        return {};
    }

    const ShaderContract::Type odtType = ShaderContract::Type::Odt;
    if (!shaderContract.validate(odtType, odtDefinition.functions())) {
        d.error = core::Error("renderengine", "odt shader does not satisfy " + shaderContract.name(odtType));
        return {};
    }

    QString texUniformBlock;
    QString texCode;

    if (textureType == ImageState::TextureType::Nv12) {
        texUniformBlock = texUniformNv12;
        texCode = texCodeNv12;
    }
    else {
        texUniformBlock = texUniformTexture2D;
        texCode = texCodeTexture2D;
    }

    const int lutFirstBinding = 4;
    QList<ShaderDescriptor::ShaderParameter> lutParams;
    if (effectDefinition)
        lutParams = effectDefinition->descriptor().lutParameters();

    for (int i = 0; i < lutParams.size(); ++i) {
        texUniformBlock
            += QString("layout(binding = %1) uniform sampler3D %2;\n").arg(lutFirstBinding + i).arg(lutParams[i].name);
    }

    if (!lutParams.isEmpty())
        texCode += lutLookupCode;

    ShaderParser::Options options;
    options.injections.set("texUniform", texUniformBlock);
    options.injections.set("texCode", texCode);
    options.injections.set("texCall", texCall);
    options.injections.set("idtCode", idtDefinition.shaderCode());
    options.injections.set("odtCode", odtDefinition.shaderCode());
    options.injections.set("idtCall", shaderContract.call(idtType));
    options.injections.set("odtCall", shaderContract.call(odtType));

    if (effectDefinition) {
        const ShaderContract::Type effectType = ShaderContract::Type::Effect;
        options.injections.set("effectUniform", effectDefinition->uniformBlock(3));
        options.injections.set("effectCode", effectDefinition->shaderCode());
        options.injections.set("effectCall", shaderContract.call(effectType));
    }
    else {
        options.injections.set("effectUniform", QString());
        options.injections.set("effectCode", QString());
        options.injections.set("effectCall", QString());
    }

    ShaderParser layerParser;
    const ShaderDefinition layerDefinition = layerParser.parse(layerSource, options);
    if (!layerParser.isValid()) {
        d.error = core::Error("renderengine", "failed to parse layer shader: " + layerParser.error().message());
        return {};
    }

    const QString generatedSource = layerDefinition.shaderCode();
    d.generatedShaderSourceCache.insert(shaderKey, generatedSource);
    return generatedSource;
}

QString
RenderEnginePrivate::buildLutShaderKey(const ShaderDefinition* effectDefinition)
{
    if (!effectDefinition)
        return QString();
    QStringList parts;
    for (const auto& param : effectDefinition->descriptor().lutParameters()) {
        const QString filename = param.value.isValid() ? param.value.toString() : param.defaultValue.toString();
        parts << param.name << filename << QString::fromLatin1(fileHash(filename));
    }
    return parts.join(",");
}

QByteArray
RenderEnginePrivate::fileHash(const QString& filename)
{
    QFileInfo info(filename);
    if (!info.exists())
        return {};

    const qint64 size = info.size();
    const qint64 modified = info.lastModified().toMSecsSinceEpoch();

    auto it = d.fileCache.constFind(filename);
    if (it != d.fileCache.constEnd() && it->size == size && it->modified == modified) {
        return it->hash;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    QCryptographicHash hash(QCryptographicHash::Sha1);
    while (!file.atEnd())
        hash.addData(file.read(1024 * 1024));

    FileHash fileHash;
    fileHash.size = size;
    fileHash.modified = modified;
    fileHash.hash = hash.result().toHex();

    d.fileCache.insert(filename, fileHash);
    return fileHash.hash;
}

QByteArray
RenderEnginePrivate::textHash(const QString& text) const
{
    return QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex();
}

void
RenderEnginePrivate::resetFrameStats()
{
#if RE_STATS_ENABLED
    d.stats.reset(d.frameIndex, static_cast<int>(d.imageLayers.size()));
#endif
}

void
RenderEnginePrivate::logFrameStats() const
{
#if RE_STATS_ENABLED
    auto ms = [](qint64 ns) { return double(ns) / 1000000.0; };
    auto fps = [](qint64 ns) { return ns > 0 ? 1000000000.0 / double(ns) : 0.0; };

    qDebug().nospace() << "renderengine: frame " << d.stats.frameIndex << " layers=" << d.stats.layerCount
                       << " fps=" << fps(d.stats.renderFrameNs) << " frameMs=" << ms(d.stats.renderFrameNs)
                       << " updateMs=" << ms(d.stats.updateRenderStatesNs) << " sceneMs=" << ms(d.stats.renderSceneNs)
                       << " blitMs=" << ms(d.stats.renderBlitNs) << " blitUpdateMs=" << ms(d.stats.updateBlitNs)
                       << " texInit=" << d.stats.texturesInitialized << " texUpload=" << d.stats.textureUploads
                       << " texBytes=" << d.stats.textureUploadBytes << " globalUpd=" << d.stats.globalBufferUpdates
                       << " globalBytes=" << d.stats.globalBufferUploadBytes
                       << " effectCreate=" << d.stats.effectBufferCreates
                       << " effectUpd=" << d.stats.effectBufferUpdates
                       << " effectBytes=" << d.stats.effectBufferUploadBytes
                       << " srbCreate=" << d.stats.shaderBindingsCreated << " pipeCreate=" << d.stats.pipelinesCreated
                       << " srcHit=" << d.stats.shaderSourceCacheHits << " srcMiss=" << d.stats.shaderSourceCacheMisses
                       << " genHit=" << d.stats.generatedShaderSourceCacheHits
                       << " genMiss=" << d.stats.generatedShaderSourceCacheMisses
                       << " shaderHit=" << d.stats.shaderCacheHits << " shaderMiss=" << d.stats.shaderCacheMisses;
#endif
}

RenderEngine::RenderEngine(QObject* parent)
    : QObject(parent)
    , p(new RenderEnginePrivate())
{}

RenderEngine::~RenderEngine() = default;

bool
RenderEngine::initialized() const
{
    return p->d.valid;
}

bool
RenderEngine::initialize(const RenderContext& context, const RenderSpec& spec)
{
    if (!context.isValid())
        return false;

    QRhiRenderTarget* renderTarget = context.surface().renderTarget();
    if (!renderTarget)
        return false;

    const bool deviceChanged = p->d.deviceRhi != context.rhi();
    const bool blitContextChanged = p->d.blitState.renderPassDescriptor != renderTarget->renderPassDescriptor()
                                    || p->d.blitState.size != spec.size();

    if (!p->d.valid || deviceChanged) {
        p->reset();
        return p->init(context, spec);
    }

    if (blitContextChanged) {
        return p->updateBlitState(p->d.blitState, renderTarget, spec);
    }

    return true;
}

void
RenderEngine::render(const RenderContext& context, const RenderSpec& spec, QRhiCommandBuffer* commandBuffer)
{
    if (!p->d.valid || !context.isValid())
        return;

    p->render(context, spec, commandBuffer);
}

QSize
RenderEngine::resolution() const
{
    return p->d.resolution;
}

void
RenderEngine::setResolution(const QSize& resolution)
{
    p->d.resolution = resolution;
}

QColor
RenderEngine::background() const
{
    return p->d.background;
}

void
RenderEngine::setBackground(const QColor& background)
{
    p->d.background = background;
}

QList<ImageLayer>
RenderEngine::imageLayers() const
{
    return p->d.imageLayers;
}

void
RenderEngine::setImageLayers(const QList<ImageLayer>& imageLayers)
{
    p->d.imageLayers = imageLayers;
}


QList<RenderOutput*>
RenderEngine::renderOutputs() const
{
    return p->d.renderOutputs;
}

void
RenderEngine::setRenderOutputs(const QList<RenderOutput*>& renderOutputs)
{
    p->d.renderOutputs = renderOutputs;
}

bool
RenderEngine::isValid() const
{
    return p->d.valid;
}

void
RenderEngine::reset()
{
    p->reset();
    p->d.imageLayers.clear();
}

}  // namespace flipman::sdk::render
