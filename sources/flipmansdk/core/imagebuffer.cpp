// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <core/imagebuffer.h>

using namespace flipman::sdk;

namespace imageio {

// Copyright Contributors to the OpenImageIO project.
// Original source: https://github.com/AcademySoftwareFoundation/OpenImageIO
// Modifications made by Mikael Sundell
// Licensed under the Apache License, Version 2.0

template<typename T> struct big_enough_float {
    typedef float float_t;
};
template<> struct big_enough_float<int> {
    typedef double float_t;
};
template<> struct big_enough_float<unsigned int> {
    typedef double float_t;
};
template<> struct big_enough_float<int64_t> {
    typedef double float_t;
};
template<> struct big_enough_float<uint64_t> {
    typedef double float_t;
};
template<> struct big_enough_float<double> {
    typedef double float_t;
};

template<typename S, typename D, typename F>
inline D
scaled_conversion(const S& src, F scale, F min, F max)
{
    if (std::numeric_limits<S>::is_signed) {
        F s = static_cast<F>(src) * scale;
        s += (s < 0 ? static_cast<F>(-0.5) : static_cast<F>(0.5));
        return static_cast<D>(std::clamp(s, min, max));
    }
    else {
        return static_cast<D>(std::clamp(static_cast<F>(src) * scale + static_cast<F>(0.5), min, max));
    }
}

template<typename S, typename D>
inline D
convert_type(const S& src)
{
    if (std::is_same<S, D>::value) {
        return (D)src;
    }
    typedef typename big_enough_float<D>::float_t F;
    F scale = std::numeric_limits<S>::is_integer ? F(1) / F(std::numeric_limits<S>::max()) : F(1);
    if (std::numeric_limits<D>::is_integer) {
        F min = (F)std::numeric_limits<D>::min();
        F max = (F)std::numeric_limits<D>::max();
        scale *= max;
        return scaled_conversion<S, D, F>(src, scale, min, max);
    }
    else {
        return (D)((F)src * scale);
    }
}

template<typename S, typename D>
void
convert_type(const S* src, D* dst, size_t n, D _min, D _max)
{
    if (std::is_same<S, D>::value) {
        memcpy(dst, src, n * sizeof(D));
        return;
    }
    typedef typename big_enough_float<D>::float_t F;
    F scale = std::numeric_limits<S>::is_integer ? (F(1)) / F(std::numeric_limits<S>::max()) : F(1);
    if (std::numeric_limits<D>::is_integer) {
        F min = (F)_min;
        F max = (F)_max;
        scale *= _max;
        for (; n >= 16; n -= 16) {
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
        }
        while (n--)
            *dst++ = scaled_conversion<S, D, F>(*src++, scale, min, max);
    }
    else {
        for (; n >= 16; n -= 16) {
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
            *dst++ = (D)((*src++) * scale);
        }
        while (n--)
            *dst++ = (D)((*src++) * scale);
    }
}

template<typename S, typename D>
inline void
to_type(const S* src, D* dst, size_t n)
{
    convert_type<S, D>(src, dst, n, std::numeric_limits<D>::min(), std::numeric_limits<D>::max());
}

const float*
to_float(const void* from, float* to, int count, const core::ImageFormat::Type type)
{
    switch (type) {
    case core::ImageFormat::FLOAT: return (float*)from;
    case core::ImageFormat::UINT8: to_type((const unsigned char*)from, to, count); break;
    case core::ImageFormat::INT8: to_type((const char*)from, to, count); break;
    case core::ImageFormat::UINT16: to_type((const unsigned short*)from, to, count); break;
    case core::ImageFormat::INT16: to_type((const short*)from, to, count); break;
    case core::ImageFormat::INT32: to_type((const int*)from, to, count); break;
    case core::ImageFormat::UINT32: to_type((const unsigned int*)from, to, count); break;
    case core::ImageFormat::INT64: to_type((const long long*)from, to, count); break;
    case core::ImageFormat::UINT64: to_type((const unsigned long long*)from, to, count); break;
    //case core::ImageFormat::HALF:
    //  to_type((const half*)from, to, count);*/ break;
    case core::ImageFormat::DOUBLE: to_type((const double*)from, to, count); break;
    default: Q_ASSERT("image format not supported" && 0);
    }
    return to;
}
}  // namespace imageio

namespace flipman::sdk::core {
class ImageBufferPrivate : public QSharedData {
public:
    ImageBufferPrivate();
    ~ImageBufferPrivate();
    void alloc();
    size_t bytesize() const;
    size_t pixelsize() const;
    size_t stridesize() const;
    size_t size() const;
    static void convert(const ImageFormat& fromformat, const quint8* from, const ImageFormat& toformat, quint8* to,
                        int count);
    struct Data {
        ImageFormat format;
        QRect datawindow;
        QRect displaywindow;
        int channels;
        QByteArray data;
    };
    Data d;
};

ImageBufferPrivate::ImageBufferPrivate() {}

ImageBufferPrivate::~ImageBufferPrivate() {}

void
ImageBufferPrivate::alloc()
{
    d.data.resize(size() * pixelsize());
}

size_t
ImageBufferPrivate::bytesize() const
{
    return pixelsize() * size();
}

size_t
ImageBufferPrivate::pixelsize() const
{
    return d.format.size() * d.channels;
}

size_t
ImageBufferPrivate::stridesize() const
{
    Q_ASSERT("datawindow is empty" && !d.datawindow.isEmpty());
    return d.datawindow.width() * pixelsize();
}

size_t
ImageBufferPrivate::size() const
{
    Q_ASSERT("datawindow is empty" && !d.datawindow.isEmpty());
    return d.datawindow.width() * d.datawindow.height();
}

void
ImageBufferPrivate::convert(const ImageFormat& fromformat, const quint8* from, const ImageFormat& toformat, quint8* to,
                            int count)
{
    if (fromformat.type() == toformat.type()) {
        memcpy((void*)to, (const void*)from, count * fromformat.size());
        return;
    }
    if (toformat.type() == ImageFormat::FLOAT) {
        imageio::to_float(from, (float*)to, count, fromformat.type());
        return;
    }
    else {
        std::array<float, 4096> stackbuf;
        std::unique_ptr<float[]> tmp;
        float* buf = (count <= 4096) ? stackbuf.data() : (tmp = std::make_unique<float[]>(count)).get();
        imageio::to_float(from, buf, count, fromformat.type());

        switch (toformat.type()) {
        case core::ImageFormat::UINT8: imageio::to_type(buf, (unsigned char*)to, count); break;
        case core::ImageFormat::INT8: imageio::to_type(buf, (char*)to, count); break;
        case core::ImageFormat::UINT16: imageio::to_type(buf, (unsigned short*)to, count); break;
        case core::ImageFormat::INT16: imageio::to_type(buf, (short*)to, count); break;
        case core::ImageFormat::UINT32: imageio::to_type(buf, (unsigned int*)to, count); break;
        case core::ImageFormat::INT32: imageio::to_type(buf, (int*)to, count); break;
        case core::ImageFormat::UINT64: imageio::to_type(buf, (unsigned long long*)to, count); break;
        case core::ImageFormat::INT64: imageio::to_type(buf, (long long*)to, count); break;
        //case core::ImageFormat::HALF:
        //  to_type((const half*)from, to, count);*/ break;
        case core::ImageFormat::DOUBLE: imageio::to_type((const double*)from, to, count); break;
        default: Q_ASSERT("image format not supported" && 0);
        }
    }
}

ImageBuffer::ImageBuffer()
    : p(new ImageBufferPrivate())
{}

ImageBuffer::ImageBuffer(const QRect& datawindow, const QRect& displaywindow, const ImageFormat& format, int channels)
    : p(new ImageBufferPrivate())
{
    p->d.datawindow = datawindow;
    p->d.displaywindow = displaywindow;
    p->d.channels = channels;
    p->d.format = format;
    p->alloc();
}

ImageBuffer::ImageBuffer(const ImageBuffer& other)
    : p(other.p)
{}

ImageBuffer::~ImageBuffer() {}

ImageFormat
ImageBuffer::imageFormat() const
{
    return p->d.format;
}

QRect
ImageBuffer::dataWindow() const
{
    return p->d.datawindow;
}

QRect
ImageBuffer::displayWindow() const
{
    return p->d.displaywindow;
}

int
ImageBuffer::channels() const
{
    return p->d.channels;
}

size_t
ImageBuffer::byteSize() const
{
    return p->bytesize();
}

size_t
ImageBuffer::pixelSize() const
{
    return p->pixelsize();
}

size_t
ImageBuffer::strideSize() const
{
    return p->stridesize();
}

size_t
ImageBuffer::size() const
{
    return p->size();
}

quint8*
ImageBuffer::data() const
{
    return reinterpret_cast<quint8*>(p->d.data.data());
}

quint8*
ImageBuffer::data(const QPoint& pos) const
{
    QPoint pixel = pos - p->d.datawindow.topLeft();
    size_t offset = pixel.y() * strideSize() + pixel.x() * pixelSize();
    Q_ASSERT("offset is out of bounds!" && offset < static_cast<size_t>(p->d.data.size()));
    return reinterpret_cast<quint8*>(p->d.data.data()) + offset;
}

ImageBuffer
ImageBuffer::detach()
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
        p->d.data = QByteArray(p->d.data);
    }
    return *this;
}

bool
ImageBuffer::isValid() const
{
    return p->d.format.isValid();
}

void
ImageBuffer::reset()
{}

void
ImageBuffer::setDisplayWindow(const QRect& displaywindow)
{  // no auto-detach
    p->d.displaywindow = displaywindow;
}

ImageBuffer&
ImageBuffer::operator=(const ImageBuffer& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
ImageBuffer::operator==(const ImageBuffer& other) const
{
    return p == other.p;
}

bool
ImageBuffer::operator!=(const ImageBuffer& other) const
{
    return !(*this == other);
}

ImageBuffer
ImageBuffer::convert(const ImageBuffer& imagebuffer, ImageFormat::Type type, int channels)
{
    if (imagebuffer.imageFormat().type() == type) {
        ImageBuffer copy = imagebuffer;
        return copy.detach();
    }
    else {
        ImageBuffer copy(imagebuffer.dataWindow(), imagebuffer.displayWindow(), type, channels);
        const quint8* src = imagebuffer.data();
        quint8* dst = copy.data();

        bool contiguous = (imagebuffer.strideSize() == imagebuffer.dataWindow().width() * imagebuffer.channels()
                           && copy.strideSize() == copy.dataWindow().width() * copy.channels());

        for (int y = 0; y < imagebuffer.dataWindow().height(); ++y) {
            const quint8* from = src + y * imagebuffer.strideSize();
            quint8* to = (quint8*)dst + y * copy.strideSize();

            if (contiguous) {
                ImageBufferPrivate::convert(imagebuffer.imageFormat(), from, copy.imageFormat(), to,
                                            channels * imagebuffer.dataWindow().width());
            }
            else {
                for (int x = 0; x < imagebuffer.dataWindow().width(); ++x) {
                    ImageBufferPrivate::convert(imagebuffer.imageFormat(), from, copy.imageFormat(), to, channels);
                    from += imagebuffer.pixelSize();
                    to += copy.pixelSize();
                }
            }
        }
        return copy;
    }
}
}  // namespace flipman::sdk::core
