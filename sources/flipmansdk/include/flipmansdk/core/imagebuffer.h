// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/core/imageformat.h>
#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/render.h>
#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QRect>
#include <QString>

namespace flipman::sdk::core {

class ImageBufferPrivate;

/**
 * @class ImageBuffer
 * @brief Explicitly shared 2D image container.
 *
 * Provides data and display windows, pixel format, memory layout control,
 * pixel layout tags, and color interpretation metadata.
 */
class FLIPMANSDK_EXPORT ImageBuffer {
    Q_GADGET
public:
    /**
     * @brief Pixel component memory layout.
     */
    enum class Packing { Interleaved, Planar, BiPlanar, Packed };
    Q_ENUM(Packing)

    /**
     * @brief Chroma subsampling mode.
     */
    enum class Subsampling { None, CS420, CS422, CS444 };
    Q_ENUM(Subsampling)

    /**
     * @brief Exact pixel storage layout.
     *
     * Packing describes the broad memory organization. PixelLayout describes
     * the exact byte/component interpretation inside that organization.
     */
    enum class PixelLayout { Unknown, RGB, BGR, RGBA, BGRA, NV12, NV21, UYVY, YUYV, YVYU, VYUY, V210 };
    Q_ENUM(PixelLayout)

    /**
     * @brief Numeric range of pixel values.
     *
     * Used primarily for YCbCr formats where video range and full range require
     * different decode paths.
     */
    enum class PixelRange { Unknown, Full, Video };
    Q_ENUM(PixelRange)

    /**
     * @brief Constructs an empty ImageBuffer.
     */
    ImageBuffer();

    /**
     * Creates an image buffer descriptor.
     *
     * Pixel storage is not allocated by the constructor. Call allocate()
     * after setting packing/subsampling and before calling data() or planeData().
     */
    explicit ImageBuffer(const QRect& dataWindow, const QRect& displayWindow, const ImageFormat& format, int channels);

    /**
     * @brief Copy constructor.
     */
    ImageBuffer(const ImageBuffer& other);

    /**
     * @brief Destroys the ImageBuffer.
     */
    ~ImageBuffer();

    /**
     * @brief Allocates memory for the current image configuration.
     *
     * Must be called after setting format, packing, and subsampling,
     * and before accessing pixel data. Reallocates if already allocated.
     */
    void allocate();

    /** @name Windows */
    ///@{

    /**
     * @brief Returns the data window.
     */
    QRect dataWindow() const;

    /**
     * @brief Returns the display window.
     */
    QRect displayWindow() const;

    /**
     * @brief Sets the display window.
     */
    void setDisplayWindow(const QRect& displaywindow);

    ///@}

    /** @name Format */
    ///@{

    /**
     * @brief Returns the image format.
     */
    ImageFormat imageFormat() const;

    /**
     * @brief Returns channel count.
     */
    int channels() const;

    /**
     * @brief Returns total byte size.
     */
    size_t byteSize() const;

    /**
     * @brief Returns bytes per pixel.
     */
    size_t pixelSize() const;

    /**
     * @brief Returns bytes per row.
     */
    size_t strideSize() const;

    /**
     * @brief Returns pixel count.
     */
    size_t size() const;

    ///@}

    /** @name Layout */
    ///@{

    /**
     * @brief Returns memory packing.
     */
    Packing packing() const;

    /**
     * @brief Sets memory packing.
     */
    void setPacking(Packing packing);

    /**
     * @brief Returns chroma subsampling.
     */
    Subsampling subsampling() const;

    /**
     * @brief Sets chroma subsampling.
     */
    void setSubsampling(Subsampling subsampling);

    /**
     * @brief Returns exact pixel storage layout.
     */
    PixelLayout pixelLayout() const;

    /**
     * @brief Sets exact pixel storage layout.
     */
    void setPixelLayout(PixelLayout pixelLayout);

    /**
     * @brief Returns pixel value range.
     */
    PixelRange pixelRange() const;

    /**
     * @brief Sets pixel value range.
     */
    void setPixelRange(PixelRange pixelRange);

    ///@}

    /** @name Classification */
    ///@{

    /**
     * @brief Returns true if the image stores YCbCr or encoded video data.
     *
     * Decode-required images must be interpreted by the render/video pipeline
     * before they become RGB pixels.
     */
    bool requiresDecode() const;

    /**
     * @brief Returns true if the image layout represents a YCbCr format.
     */
    bool isYCbCr() const;

    /**
     * @brief Returns true if the image layout represents an RGB-like format.
     */
    bool isRgb() const;

    ///@}

    /** @name Color Interpretation */
    ///@{

    /**
     * @brief Returns source color space tag.
     */
    render::ColorSpace colorSpace() const;

    /**
     * @brief Sets source color space tag.
     */
    void setColorSpace(render::ColorSpace colorSpace);

    /**
     * @brief Returns source transfer function tag.
     */
    render::TransferFunction transferFunction() const;

    /**
     * @brief Sets source transfer function tag.
     */
    void setTransferFunction(render::TransferFunction transferFunction);

    ///@}

    /** @name Data Access */
    ///@{

    /**
     * @brief Returns pointer to raw data.
     */
    quint8* data() const;

    /**
     * @brief Returns pointer to pixel at position.
     */
    quint8* data(const QPoint& pos) const;

    /**
     * @brief Returns number of planes.
     */
    int planeCount() const;

    /**
     * @brief Returns row stride for a plane.
     */
    size_t planeStride(int plane) const;

    /**
     * @brief Returns logical size of a plane.
     */
    QSize planeSize(int plane) const;

    /**
     * @brief Returns total byte size of a plane.
     */
    size_t planeByteSize(int plane) const;

    /**
     * @brief Returns pointer to a plane.
     */
    quint8* planeData(int plane) const;

    /**
     * @brief Detaches shared data.
     */
    ImageBuffer detach();

    /**
     * @brief Returns true if backing memory is allocated.
     *
     * Required before accessing pixel data.
     */
    bool isAllocated() const;

    /**
     * @brief Returns true if valid.
     */
    bool isValid() const;

    /**
     * @brief Resets to empty state.
     */
    void reset();

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    ImageBuffer& operator=(const ImageBuffer& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const ImageBuffer& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const ImageBuffer& other) const;

    ///@}

    /**
     * @brief Converts an image buffer to a different pixel format and channel count.
     *
     * Returns a new ImageBuffer where the pixel data is converted to the
     * specified type and number of channels.
     */
    static ImageBuffer convert(const ImageBuffer& imagebuffer, ImageFormat::Type type, int channels);

    /**
     * @brief Converts an image buffer to a different channel count.
     *
     * Returns a new ImageBuffer with the same pixel format but a different
     * number of channels.
     */
    static ImageBuffer convert(const ImageBuffer& imageBuffer, int channels);

private:
    QExplicitlySharedDataPointer<ImageBufferPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::ImageBuffer)
Q_DECLARE_METATYPE(flipman::sdk::core::ImageBuffer::Packing)
Q_DECLARE_METATYPE(flipman::sdk::core::ImageBuffer::Subsampling)
Q_DECLARE_METATYPE(flipman::sdk::core::ImageBuffer::PixelLayout)
Q_DECLARE_METATYPE(flipman::sdk::core::ImageBuffer::PixelRange)
