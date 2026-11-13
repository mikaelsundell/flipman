// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "core/private/imageconversion.h"
#include "private/rhiimageconverter.h"

#include <flipmansdk/render/imageconverter.h>
#include <memory>

namespace flipman::sdk::render {

class ImageConverterPrivate {
public:
    struct Data {
        core::Error error;
        std::unique_ptr<RhiImageConverter> gpu;
    };
    Data d;
};

ImageConverter::ImageConverter(QRhi* rhi)
    : p(new ImageConverterPrivate)
{
    if (rhi)
        p->d.gpu = std::make_unique<RhiImageConverter>(rhi);
}

ImageConverter::~ImageConverter() = default;

core::ImageBuffer
ImageConverter::convertCpu(const core::ImageBuffer& source, core::ImageFormat::Type type, int channels)
{
    return core::detail::convertImage(source, type, channels);
}

core::ImageBuffer
ImageConverter::convertCpu(const core::ImageBuffer& source, int channels)
{
    return core::detail::convertChannels(source, channels);
}

bool
ImageConverter::record(QRhiCommandBuffer* commands, const core::ImageBuffer& source, QRhiTextureRenderTarget* target)
{
    if (!p->d.gpu) {
        p->d.error = core::Error("imageconverter", "GPU recording requires a borrowed QRhi device");
        return false;
    }
    return p->d.gpu->convert(commands, source, target);
}

bool
ImageConverter::record(QRhiCommandBuffer* commands, QRhiTexture* source, QRhiTextureRenderTarget* target)
{
    if (!p->d.gpu) {
        p->d.error = core::Error("imageconverter", "GPU recording requires a borrowed QRhi device");
        return false;
    }
    return p->d.gpu->convert(commands, source, target);
}

bool
ImageConverter::convert(QRhiCommandBuffer* commands, const core::ImageBuffer& source, QRhiTextureRenderTarget* target)
{
    return record(commands, source, target);
}

bool
ImageConverter::convert(QRhiCommandBuffer* commands, QRhiTexture* source, QRhiTextureRenderTarget* target)
{
    return record(commands, source, target);
}

void
ImageConverter::releaseResources()
{
    if (p->d.gpu)
        p->d.gpu->releaseResources();
}

core::Error
ImageConverter::error() const
{
    return p->d.gpu ? p->d.gpu->error() : p->d.error;
}

}  // namespace flipman::sdk::render
