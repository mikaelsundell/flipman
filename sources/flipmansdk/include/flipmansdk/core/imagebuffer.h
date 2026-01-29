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
 * @brief Represents a multi-channel 2D image container with explicit data sharing.
 * * ImageBuffer manages raw pixel data using a "Data Window" (actual pixels in memory)
 * and a "Display Window" (the viewable area), supporting high-performance
 * imaging workflows and zero-copy passing.
 */
class FLIPMANSDK_EXPORT ImageBuffer {
public:
    /// Constructs an empty, invalid image buffer.
    ImageBuffer();

    /**
     * @brief Constructs a buffer with specified dimensions and format.
     * @param datawindow The actual pixel bounds in memory.
     * @param displaywindow The viewable area of the image.
     * @param format The pixel data type (e.g., FLOAT, UINT8).
     * @param channels The number of color/alpha channels.
     */
    ImageBuffer(const QRect& datawindow, const QRect& displaywindow, const ImageFormat& format, int channels);

    /// Copy constructor (shallow copy via shared data pointer).
    ImageBuffer(const ImageBuffer& other);

    virtual ~ImageBuffer();

    /// Returns the pixel data format.
    ImageFormat imageFormat() const;

    /// Returns the pixel bounds of the actual data stored in memory.
    QRect dataWindow() const;

    /// Returns the bounds of the image as it should be displayed.
    QRect displayWindow() const;

    /// Returns the number of channels per pixel.
    int channels() const;

    /// Returns the total size of the buffer in bytes.
    size_t byteSize() const;

    /// Returns the byte size of a single pixel (channels * format size).
    size_t pixelSize() const;

    /// Returns the byte size of a single scanline (width * pixelSize).
    size_t strideSize() const;

    /// Returns the total number of pixels in the data window.
    size_t size() const;

    /// Returns a pointer to the start of the raw pixel data.
    quint8* data() const;

    /// Returns a pointer to the pixel data at a specific coordinate.
    quint8* data(const QPoint& pos) const;

    /**
     * @brief Creates a deep copy of the buffer if the data is shared.
     * @return A reference to the detached buffer.
     */
    ImageBuffer detach();

    /// Returns true if the buffer contains valid data.
    bool isValid() const;

    /// Resets the buffer to an uninitialized state.
    void reset();

    /// Updates the display window without modifying the underlying pixel data.
    void setDisplayWindow(const QRect& displaywindow);

    ImageBuffer& operator=(const ImageBuffer& other);
    bool operator==(const ImageBuffer& other) const;
    bool operator!=(const ImageBuffer& other) const;

    /**
     * @brief Returns a copy of the buffer converted to a different format or channel count.
     * @param imagebuffer The source buffer.
     * @param type The target pixel type.
     * @param channels The target channel count.
     */
    static ImageBuffer convert(const ImageBuffer& imagebuffer, ImageFormat::Type type, int channels);

private:
    QExplicitlySharedDataPointer<ImageBufferPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::ImageBuffer)
