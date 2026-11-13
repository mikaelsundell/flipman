// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "private/ociogpu.h"

#include <flipmansdk/core/profile.h>
#include <QRegularExpression>
#include <cstring>
#include <stdexcept>

namespace flipman::sdk::render {

void
OcioGpu::build(QRhi* rhi, const ocio::ConstProcessorRcPtr& processor, const char* function, int firstBinding)
{
    FM_PROFILE_SCOPE("color", "gpu.prepare");
    auto desc = ocio::GpuShaderDesc::CreateShaderDesc();
    desc->setLanguage(ocio::GPU_LANGUAGE_GLSL_4_0);
    desc->setFunctionName(function);
    const QByteArray prefix = QByteArray(function) + "_";
    desc->setResourcePrefix(prefix.constData());
    desc->setAllowTexture1D(false);
    desc->setTextureMaxWidth(4096);
    processor->getDefaultGPUProcessor()->extractGpuShaderInfo(desc);
    if (desc->getNumUniforms())
        throw std::runtime_error("dynamic OCIO uniforms are not supported; use a static config transform");
    shader = QString::fromUtf8(desc->getShaderText());
    textures.clear();

    const auto add = [&](const char* sampler, unsigned width, unsigned height, int edge, int channels,
                         const float* values, ocio::Interpolation interpolation) {
        if (!width || !height || !values || width > 4096 || height > 4096 || edge > 256)
            throw std::runtime_error("unsupported OCIO LUT dimensions");
        OcioTexture lut;
        lut.binding = firstBinding + int(textures.size());
        lut.edge = edge;
        const qsizetype count = qsizetype(width) * height * (edge ? edge : 1);
        lut.data.resize(count * 4 * sizeof(float));
        auto* dst = reinterpret_cast<float*>(lut.data.data());

        // Only LUT data is expanded, once at transform setup. Never image pixels.
        for (qsizetype i = 0; i < count; ++i) {
            for (int c = 0; c < 3; ++c)
                dst[4 * i + c] = values[channels * i + (channels == 1 ? 0 : c)];
            dst[4 * i + 3] = 1.0f;
        }
        if (edge)
            lut.texture.reset(
                rhi->newTexture(QRhiTexture::RGBA32F, int(width), int(height), edge, 1, QRhiTexture::ThreeDimensional));
        else
            lut.texture.reset(rhi->newTexture(QRhiTexture::RGBA32F, QSize(int(width), int(height))));

        if (!lut.texture->create())
            throw std::runtime_error("could not allocate OCIO LUT texture");

        const auto filter = interpolation == ocio::INTERP_NEAREST ? QRhiSampler::Nearest : QRhiSampler::Linear;
        lut.sampler.reset(rhi->newSampler(filter, filter, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                          QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        if (!lut.sampler->create())
            throw std::runtime_error("could not allocate OCIO LUT sampler");
        const QRegularExpression declaration(QStringLiteral("uniform\\s+(sampler[23]D)\\s+%1\\s*;")
                                                 .arg(QRegularExpression::escape(QString::fromUtf8(sampler))));

        if (!declaration.match(shader).hasMatch())
            throw std::runtime_error("OCIO LUT sampler declaration not found");

        shader.replace(declaration, QString("layout(binding = %1) uniform \\1 %2;").arg(lut.binding).arg(sampler));
        textures.push_back(std::move(lut));
    };
    for (unsigned i = 0; i < desc->getNumTextures(); ++i) {
        const char *name = nullptr, *sampler = nullptr;
        unsigned width = 0, height = 0;
        ocio::GpuShaderCreator::TextureType channel;
        ocio::GpuShaderCreator::TextureDimensions dimensions;
        ocio::Interpolation interpolation;
        desc->getTexture(i, name, sampler, width, height, channel, dimensions, interpolation);
        const float* values = nullptr;
        desc->getTextureValues(i, values);
        if (dimensions != ocio::GpuShaderCreator::TEXTURE_2D)
            throw std::runtime_error("OCIO generated a 1D texture despite 2D-only settings");
        add(sampler, width, height, 0, channel == ocio::GpuShaderCreator::TEXTURE_RED_CHANNEL ? 1 : 3, values,
            interpolation);
    }
    for (unsigned i = 0; i < desc->getNum3DTextures(); ++i) {
        const char *name = nullptr, *sampler = nullptr;
        unsigned edge = 0;
        ocio::Interpolation interpolation;
        desc->get3DTexture(i, name, sampler, edge, interpolation);
        const float* values = nullptr;
        desc->get3DTextureValues(i, values);
        add(sampler, edge, edge, int(edge), 3, values, interpolation);
    }
    uploaded = false;
}

void
OcioGpu::upload(QRhiResourceUpdateBatch* updates)
{
    if (uploaded)
        return;

    for (auto& lut : textures) {
        QVector<QRhiTextureUploadEntry> entries;
        const int slices = lut.edge ? lut.edge : 1;
        const qsizetype bytes = lut.data.size() / slices;
        for (int z = 0; z < slices; ++z) {
            QRhiTextureSubresourceUploadDescription slice(lut.data.constData() + z * bytes, quint32(bytes));
            slice.setSourceSize(lut.texture->pixelSize());
            entries.append(QRhiTextureUploadEntry(z, 0, slice));
        }
        QRhiTextureUploadDescription upload;
        upload.setEntries(entries.cbegin(), entries.cend());
        updates->uploadTexture(lut.texture.get(), upload);
        lut.data.clear();
    }
    uploaded = true;
}

void
OcioGpu::bindings(QVector<QRhiShaderResourceBinding>& bindings) const
{
    for (const auto& lut : textures)
        bindings << QRhiShaderResourceBinding::sampledTexture(lut.binding, QRhiShaderResourceBinding::FragmentStage,
                                                              lut.texture.get(), lut.sampler.get());
}

}  // namespace flipman::sdk::render
