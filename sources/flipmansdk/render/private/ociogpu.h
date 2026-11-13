// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/render/colorpipeline.h>
#include <OpenColorIO/OpenColorIO.h>
#include <QByteArray>
#include <QString>
#include <rhi/qrhi.h>
#include <memory>
#include <vector>

namespace flipman::sdk::render {

namespace ocio = OCIO_NAMESPACE;
ocio::ConstConfigRcPtr
ocioConfig(const QString& filename);
ocio::ConstProcessorRcPtr
ocioOutputProcessor(const ocio::ConstConfigRcPtr& config, const ColorSettings& settings);

struct OcioTexture {
    int binding = 0;
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiSampler> sampler;
    QByteArray data;
    int edge = 0;
};

// Per-device resources. Shader/processor preparation and LUT uploads happen only
// when settings change; frame submission only binds the cached resources.
struct OcioGpu {
    QString key;
    QString shader;
    std::vector<OcioTexture> textures;
    bool uploaded = false;
    void build(QRhi* rhi, const ocio::ConstProcessorRcPtr& processor, const char* function, int firstBinding);
    void upload(QRhiResourceUpdateBatch* updates);
    void bindings(QVector<QRhiShaderResourceBinding>& bindings) const;
};

}  // namespace flipman::sdk::render
