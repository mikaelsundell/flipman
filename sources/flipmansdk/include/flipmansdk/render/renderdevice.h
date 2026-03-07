// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/render/renderengine.h>

#include <QImage>
#include <QObject>
#include <QScopedPointer>
#include <QSize>

#include <rhi/qrhi.h>

namespace flipman::sdk::render {

class RenderDevicePrivate;

/**
 * @class RenderDevice
 * @brief Low-level GPU device abstraction.
 *
 * Owns QRhi resources and manages frame lifecycle.
 * Does not perform rendering logic.
 */
class FLIPMANSDK_EXPORT RenderDevice : public QObject {
    Q_OBJECT
public:
    /**
     * @enum Backend
     * @brief Rendering backend used by the device.
     *
     * Determines which graphics API is used internally. Backend::Auto
     * selects the most appropriate backend for the current platform.
     */
    enum Backend {
        Auto,      ///< Automatically select best backend for platform.
        Metal,     ///< Apple Metal.
        Vulkan,    ///< Vulkan.
        Direct3D,  ///< Direct3D 11.
        OpenGL,    ///< OpenGL / GLES.
        Null       ///< No-op backend (testing only).
    };
    Q_ENUM(Backend)

    /**
     * @enum RenderTargetFormat
     * @brief Pixel format of the render target.
     */
    enum RenderTargetFormat {
        Rgba8,    ///< 8-bit UNORM.
        Rgba16F,  ///< 16-bit floating point.
        Rgba32F   ///< 32-bit floating point.
    };
    Q_ENUM(RenderTargetFormat)


public:
    /**
     * @brief Constructs an uninitialized RenderDevice.
     */
    explicit RenderDevice(QObject* parent = nullptr);

    /**
     * @brief Destroys the RenderDevice and releases GPU resources.
     */
    ~RenderDevice() override;

    /**
     * @brief Creates an offscreen rendering device.
     *
     * Initializes the GPU device and render target resources.
     *
     * @param backend Rendering backend to use. Backend::Auto selects
     *        the most appropriate backend for the current platform.
     * @param size Render target resolution.
     * @param renderTargetFormat Render target pixel format.
     * @return true if creation succeeded.
     */
    bool create(Backend backend, const QSize& size,
                RenderTargetFormat renderTargetFormat = RenderTargetFormat::Rgba16F);

    /**
     * @brief Begins a rendering frame.
     *
     * @param cb Output command buffer.
     * @return true if frame started successfully.
     */
    bool beginFrame(QRhiCommandBuffer*& cb);

    /**
     * @brief Ends the current rendering frame.
     */
    void endFrame();

    /**
     * @brief Returns the current RenderEngine context.
     */
    RenderEngine::Context context() const;

    /**
     * @brief Returns the render target pixel format.
     */
    RenderTargetFormat renderTargetFormat() const;

    /**
     * @brief Returns the active rendering backend.
     *
     * If Backend::Auto was used during creation, this returns the
     * backend selected internally for the platform.
     */
    Backend backend() const;

    /**
     * @brief Reads back the rendered image from the GPU.
     *
     * @return Rendered image.
     */
    core::ImageBuffer readback() const;

    /**
     * @brief Returns the render target resolution.
     */
    QSize size() const;

    /**
     * @brief Returns the last error encountered during compilation.
     */
    core::Error error() const;

    /**
     * @brief Returns true if the interpreter is usable.
     */
    bool isValid() const;

private:
    Q_DISABLE_COPY_MOVE(RenderDevice)
    QScopedPointer<RenderDevicePrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::RenderDevice*)
