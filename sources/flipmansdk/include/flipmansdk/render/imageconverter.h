// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/flipmansdk.h>
#include <QScopedPointer>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

class ImageConverterPrivate;

/**
 * @brief
 * Provides explicit CPU conversion and GPU recording through shared implementations.
 *
 * For GPU use, the device must outlive this object; use and destroy it on
 * the QRhi owning thread. Resources and pipelines are cached. GPU recording
 * performs no color transform, CPU pixel conversion, readback or GPU wait.
 * CPU operations use the shared core kernels and require no rendering device.
 * Record one conversion per instance per frame; use separate instances for
 * independent conversions within a frame. Finish pending GPU work before
 * resizing source resources or changing the destination format.
 */
class FLIPMANSDK_EXPORT ImageConverter {
public:
    /**
     * @brief
     * Borrows a device for GPU recording, or accepts null for CPU-only use.
     *
     * No device or queue is created. GPU recording never falls back to CPU.
     */
    explicit ImageConverter(QRhi* rhi = nullptr);

    /**
     * @brief
     * Converts CPU pixels using portable scalar or architecture-specific kernels.
     *
     * No GPU device, upload or readback is used. Layout and color tags retain
     * ImageBuffer::convert semantics; this does not reorder RGB/BGR channels.
     * A format-and-channel identity request returns detached storage.
     */
    static core::ImageBuffer convertCpu(const core::ImageBuffer& source, core::ImageFormat::Type type, int channels);

    /**
     * @brief
     * Changes CPU channel count, using NEON where available for RGB-to-RGBA.
     *
     * A channel-only identity request retains shared storage. The portable
     * implementation is also available on macOS; CPU choice is not OS-specific.
     */
    static core::ImageBuffer convertCpu(const core::ImageBuffer& source, int channels);

    /**
     * @brief
     * Releases conversion resources before the borrowed device is destroyed.
     */
    ~ImageConverter();

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
    bool record(QRhiCommandBuffer* commands, const core::ImageBuffer& source, QRhiTextureRenderTarget* target);

    /**
     * @brief
     * Records conversion of GPU-resident RGBA8/BGRA8/RGBA16F/RGBA32F pixels.
     *
     * No source upload occurs. Source and destination must be different textures
     * on this device. Keep a texture source alive until this converter is
     * destroyed or releaseResources() is called, and until GPU completion.
     * Destination resources must remain alive until GPU completion. No resizing occurs.
     */
    bool record(QRhiCommandBuffer* commands, QRhiTexture* source, QRhiTextureRenderTarget* target);

    /**
     * @brief
     * Compatibility forwards to record(); GPU completion remains caller-owned.
     */
    bool convert(QRhiCommandBuffer* commands, const core::ImageBuffer& source, QRhiTextureRenderTarget* target);

    /**
     * @brief
     * Compatibility forward for GPU texture recording.
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
    Q_DISABLE_COPY_MOVE(ImageConverter)
    QScopedPointer<ImageConverterPrivate> p;
};

}  // namespace flipman::sdk::render
