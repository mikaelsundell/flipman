// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/imageformat.h>

#include <QExplicitlySharedDataPointer>
#include <QRect>
#include <QString>


namespace core {
class ImageBufferPrivate;
class ImageBuffer : public Object {
public:
    ImageBuffer();
    ImageBuffer(const QRect& datawindow, const QRect& displaywindow, const ImageFormat& format, int channels);
    ImageBuffer(const ImageBuffer& other);
    virtual ~ImageBuffer();
    bool is_valid() const override;
    ImageFormat imageformat() const;
    QRect datawindow() const;
    QRect displaywindow() const;
    int channels() const;
    size_t bytesize() const;
    size_t pixelsize() const;
    size_t stridesize() const;
    size_t size() const;
    quint8* data() const;
    quint8* data(const QPoint& pos) const;
    ImageBuffer detach();
    void reset() override;

    void set_displaywindow(const QRect& displaywindow);

    ImageBuffer& operator=(const ImageBuffer& other);
    bool operator==(const ImageBuffer& other) const;
    bool operator!=(const ImageBuffer& other) const;

    static ImageBuffer convert(const ImageBuffer& imagebuffer, ImageFormat::Type type, int channels);

private:
    QExplicitlySharedDataPointer<ImageBufferPrivate> p;
};
}  // namespace core
