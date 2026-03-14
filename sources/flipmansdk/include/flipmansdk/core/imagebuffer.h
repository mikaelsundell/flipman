// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/imageformat.h>

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
 * Provides data and display windows, pixel format, and memory layout control.
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
     * @brief Constructs an empty ImageBuffer.
     */
    ImageBuffer();

    /**
     * @brief Constructs a buffer with dimensions and format.
     */
    explicit ImageBuffer(const QRect& datawindow, const QRect& displaywindow, const ImageFormat& format, int channels);

    /**
     * @brief Copy constructor.
     */
    ImageBuffer(const ImageBuffer& other);

    /**
     * @brief Destroys the ImageBuffer.
     */
    ~ImageBuffer();

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
