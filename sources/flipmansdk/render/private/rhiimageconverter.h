// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/flipmansdk.h>
#include <QScopedPointer>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

class RhiImageConverterPrivate;

/**
 * @brief
 * Records GPU pixel-format/layout conversion into a caller-owned QRhi frame.
 *
 * The device must outlive this object. Use and destroy it on
 * the QRhi owning thread. Resources and pipelines are cached. The converter
 * performs no color transform, CPU pixel conversion, readback or GPU wait.
 * Record one conversion per instance per frame; use separate instances for
 * independent conversions within a frame. Finish pending GPU work before
 * resizing source resources or changing the destination format.
 */
class RhiImageConverter {
public:
    /**
     * @brief
     * Borrows an existing device; does not create another GPU device or queue.
     */
    explicit RhiImageConverter(QRhi* rhi);
    /**
     * @brief
     * Releases conversion resources before the borrowed device is destroyed.
     */
    ~RhiImageConverter();
    /**
     * @brief
     * Uploads CPU pixels once and records conversion into a single-color target.
     *
     * Supports interleaved RGB/RGBA/BGRA UInt8/Half/Float. A three-channel source
     * gets alpha 1. Output is RGBA8/BGRA8/RGBA16F/RGBA32F, at the source size.
     * Input data is copied into QRhi's upload batch during this call; the caller
     * may release it afterwards. This ingress upload is unnecessary for the
     * texture overload. Source color tags are not interpreted or transformed.
     */
    bool convert(QRhiCommandBuffer* commands, const core::ImageBuffer& source, QRhiTextureRenderTarget* target);
    /**
     * @brief
     * Records conversion of GPU-resident RGBA8/BGRA8/RGBA16F/RGBA32F pixels.
     *
     * No source upload occurs. Source and destination must be different textures
     * on this device. Keep a texture source alive until this converter is
     * destroyed or releaseResources() is called, and until GPU completion.
     * Destination resources must remain alive until GPU completion. No resizing occurs.
     */
    bool convert(QRhiCommandBuffer* commands, QRhiTexture* source, QRhiTextureRenderTarget* target);
    /**
     * @brief
     * Releases cached resources after the caller has completed pending GPU work.
     *
     * Call before destroying a borrowed source texture. Shader packages remain
     * cached; the next conversion recreates resources on the owning thread.
     */
    void releaseResources();
    /**
     * @brief
     * Returns the last validation, shader compilation or resource error.
     */
    core::Error error() const;

private:
    Q_DISABLE_COPY_MOVE(RhiImageConverter)
    QScopedPointer<RhiImageConverterPrivate> p;
};

}  // namespace flipman::sdk::render
