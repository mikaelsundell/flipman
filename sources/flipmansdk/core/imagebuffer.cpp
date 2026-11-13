// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "private/imageconversion.h"

#include <flipmansdk/core/imagebuffer.h>

namespace flipman::sdk::core {

class ImageBufferPrivate : public QSharedData {
public:
    ImageBufferPrivate();
    ~ImageBufferPrivate();
    void alloc();
    size_t byteSize() const;
    size_t pixelSize() const;
    size_t strideSize() const;
    size_t size() const;

    struct Data {
        ImageFormat format;
        ImageBuffer::Packing packing = ImageBuffer::Packing::Interleaved;
        ImageBuffer::Subsampling subsampling = ImageBuffer::Subsampling::None;
        ImageBuffer::PixelLayout pixelLayout = ImageBuffer::PixelLayout::Unknown;
        ImageBuffer::PixelRange pixelRange = ImageBuffer::PixelRange::Unknown;
        render::ColorSpace colorSpace = render::ColorSpace::Unknown;
        render::TransferFunction transferFunction = render::TransferFunction::Unknown;
        QRect dataWindow;
        QRect displayWindow;
        QByteArray data;
        int channels = 0;
    };
    Data d;
};

ImageBufferPrivate::ImageBufferPrivate() {}

ImageBufferPrivate::~ImageBufferPrivate() {}

void
ImageBufferPrivate::alloc()
{
    const int w = d.dataWindow.width();
    const int h = d.dataWindow.height();
    const size_t comp = d.format.size();

    size_t total = 0;

    switch (d.packing) {
    case ImageBuffer::Packing::Interleaved:
    case ImageBuffer::Packing::Packed:
    case ImageBuffer::Packing::Planar: total = size_t(w) * size_t(h) * size_t(d.channels) * comp; break;

    case ImageBuffer::Packing::BiPlanar: {
        size_t y = size_t(w) * size_t(h) * comp;

        int cw = w;
        int ch = h;

        if (d.subsampling == ImageBuffer::Subsampling::CS420) {
            cw /= 2;
            ch /= 2;
        }
        else if (d.subsampling == ImageBuffer::Subsampling::CS422) {
            cw /= 2;
        }

        size_t uv = size_t(cw) * size_t(ch) * 2 * comp;
        total = y + uv;
        break;
    }

    default: total = 0; break;
    }

    d.data.resize(qsizetype(total));
}

size_t
ImageBufferPrivate::byteSize() const
{
    if (!d.data.isEmpty())
        return size_t(d.data.size());

    switch (d.packing) {
    case ImageBuffer::Packing::Interleaved:
    case ImageBuffer::Packing::Packed:
    case ImageBuffer::Packing::Planar:
        return size_t(d.dataWindow.width()) * size_t(d.dataWindow.height()) * size_t(d.channels) * d.format.size();

    case ImageBuffer::Packing::BiPlanar: {
        const int w = d.dataWindow.width();
        const int h = d.dataWindow.height();

        int cw = w;
        int ch = h;

        if (d.subsampling == ImageBuffer::Subsampling::CS420) {
            cw /= 2;
            ch /= 2;
        }
        else if (d.subsampling == ImageBuffer::Subsampling::CS422) {
            cw /= 2;
        }

        const size_t y = size_t(w) * size_t(h) * d.format.size();
        const size_t uv = size_t(cw) * size_t(ch) * 2 * d.format.size();

        return y + uv;
    }
    }

    return 0;
}

size_t
ImageBufferPrivate::pixelSize() const
{
    return d.format.size() * size_t(d.channels);
}

size_t
ImageBufferPrivate::strideSize() const
{
    Q_ASSERT(!d.dataWindow.isEmpty() && "datawindow is empty");
    return size_t(d.dataWindow.width()) * pixelSize();
}

size_t
ImageBufferPrivate::size() const
{
    Q_ASSERT(!d.dataWindow.isEmpty() && "datawindow is empty");
    return size_t(d.dataWindow.width()) * size_t(d.dataWindow.height());
}

ImageBuffer::ImageBuffer()
    : p(new ImageBufferPrivate())
{}

ImageBuffer::ImageBuffer(const QRect& datawindow, const QRect& displaywindow, const ImageFormat& format, int channels)
    : p(new ImageBufferPrivate())
{
    p->d.dataWindow = datawindow;
    p->d.displayWindow = displaywindow;
    p->d.channels = channels;
    p->d.format = format;
}

ImageBuffer::ImageBuffer(const ImageBuffer& other)
    : p(other.p)
{}

ImageBuffer::~ImageBuffer() {}

void
ImageBuffer::allocate()
{
    detach();
    p->alloc();
}

ImageFormat
ImageBuffer::imageFormat() const
{
    return p->d.format;
}

QRect
ImageBuffer::dataWindow() const
{
    return p->d.dataWindow;
}

QRect
ImageBuffer::displayWindow() const
{
    return p->d.displayWindow;
}

void
ImageBuffer::setDisplayWindow(const QRect& displayWindow)
{
    detach();
    p->d.displayWindow = displayWindow;
}

int
ImageBuffer::channels() const
{
    return p->d.channels;
}

size_t
ImageBuffer::byteSize() const
{
    return p->byteSize();
}

size_t
ImageBuffer::pixelSize() const
{
    return p->pixelSize();
}

size_t
ImageBuffer::strideSize() const
{
    return p->strideSize();
}

size_t
ImageBuffer::size() const
{
    return p->size();
}

ImageBuffer::Packing
ImageBuffer::packing() const
{
    return p->d.packing;
}

void
ImageBuffer::setPacking(Packing packing)
{
    if (p->d.packing != packing) {
        detach();

        p->d.packing = packing;

        if (packing == Packing::Interleaved) {
            p->d.subsampling = Subsampling::None;
        }

        p->d.data.clear();
    }
}

ImageBuffer::Subsampling
ImageBuffer::subsampling() const
{
    return p->d.subsampling;
}

void
ImageBuffer::setSubsampling(Subsampling subsampling)
{
    Q_ASSERT(subsampling == Subsampling::None || p->d.packing == Packing::Planar || p->d.packing == Packing::BiPlanar
             || p->d.packing == Packing::Packed);

    if (p->d.subsampling != subsampling) {
        detach();
        p->d.subsampling = subsampling;
        p->d.data.clear();
    }
}

ImageBuffer::PixelLayout
ImageBuffer::pixelLayout() const
{
    return p->d.pixelLayout;
}

void
ImageBuffer::setPixelLayout(PixelLayout pixelLayout)
{
    if (p->d.pixelLayout != pixelLayout) {
        detach();
        p->d.pixelLayout = pixelLayout;
    }
}

ImageBuffer::PixelRange
ImageBuffer::pixelRange() const
{
    return p->d.pixelRange;
}

void
ImageBuffer::setPixelRange(PixelRange pixelRange)
{
    if (p->d.pixelRange != pixelRange) {
        detach();
        p->d.pixelRange = pixelRange;
    }
}

bool
ImageBuffer::requiresDecode() const
{
    if (isYCbCr())
        return true;

    return p->d.packing == Packing::Planar || p->d.packing == Packing::BiPlanar || p->d.packing == Packing::Packed;
}

bool
ImageBuffer::isYCbCr() const
{
    switch (p->d.pixelLayout) {
    case PixelLayout::NV12:
    case PixelLayout::NV21:
    case PixelLayout::UYVY:
    case PixelLayout::YUYV:
    case PixelLayout::YVYU:
    case PixelLayout::VYUY:
    case PixelLayout::V210: return true;

    default: return false;
    }
}

bool
ImageBuffer::isRgb() const
{
    switch (p->d.pixelLayout) {
    case PixelLayout::RGB:
    case PixelLayout::BGR:
    case PixelLayout::RGBA:
    case PixelLayout::BGRA: return true;

    case PixelLayout::Unknown:
        return p->d.packing == Packing::Interleaved && p->d.subsampling == Subsampling::None
               && (p->d.channels == 3 || p->d.channels == 4);

    default: return false;
    }
}

render::ColorSpace
ImageBuffer::colorSpace() const
{
    return p->d.colorSpace;
}

void
ImageBuffer::setColorSpace(render::ColorSpace colorSpace)
{
    if (p->d.colorSpace != colorSpace) {
        detach();
        p->d.colorSpace = colorSpace;
    }
}

render::TransferFunction
ImageBuffer::transferFunction() const
{
    return p->d.transferFunction;
}

void
ImageBuffer::setTransferFunction(render::TransferFunction transferFunction)
{
    if (p->d.transferFunction != transferFunction) {
        detach();
        p->d.transferFunction = transferFunction;
    }
}

quint8*
ImageBuffer::data() const
{
    Q_ASSERT(!p->d.data.isEmpty() && "imagebuffer not allocated. Call allocate() before accessing data.");
    return reinterpret_cast<quint8*>(p->d.data.data());
}

quint8*
ImageBuffer::data(const QPoint& pos) const
{
    Q_ASSERT(!p->d.data.isEmpty() && "imagebuffer not allocated. Call allocate() before accessing data.");
    Q_ASSERT(p->d.packing == Packing::Interleaved);

    QPoint pixel = pos - p->d.dataWindow.topLeft();
    size_t offset = size_t(pixel.y()) * strideSize() + size_t(pixel.x()) * pixelSize();

    Q_ASSERT(offset < static_cast<size_t>(p->d.data.size()));
    return reinterpret_cast<quint8*>(p->d.data.data()) + offset;
}

int
ImageBuffer::planeCount() const
{
    switch (p->d.packing) {
    case Packing::Interleaved:
    case Packing::Packed: return 1;

    case Packing::Planar: return p->d.channels;

    case Packing::BiPlanar: return 2;

    default: return 0;
    }
}

size_t
ImageBuffer::planeStride(int plane) const
{
    Q_ASSERT(plane >= 0 && plane < planeCount());

    const int width = p->d.dataWindow.width();

    switch (p->d.packing) {
    case Packing::Interleaved:
    case Packing::Packed: return size_t(width) * pixelSize();

    case Packing::Planar: return size_t(width) * p->d.format.size();

    case Packing::BiPlanar:
        if (plane == 0) {
            return size_t(width) * p->d.format.size();
        }
        else {
            int chromaWidth = width;

            if (p->d.subsampling == Subsampling::CS420 || p->d.subsampling == Subsampling::CS422)
                chromaWidth /= 2;

            return size_t(chromaWidth) * 2 * p->d.format.size();
        }
    }

    return 0;
}

QSize
ImageBuffer::planeSize(int plane) const
{
    Q_ASSERT(plane >= 0 && plane < planeCount() && "planeSize: plane index out of range.");

    const int width = p->d.dataWindow.width();
    const int height = p->d.dataWindow.height();

    switch (p->d.packing) {
    case Packing::Interleaved:
    case Packing::Packed: return QSize(width, height);

    case Packing::Planar: return QSize(width, height);

    case Packing::BiPlanar:
        if (plane == 0) {
            return QSize(width, height);
        }
        else {
            int chromaWidth = width;
            int chromaHeight = height;

            switch (p->d.subsampling) {
            case Subsampling::CS420:
                chromaWidth /= 2;
                chromaHeight /= 2;
                break;

            case Subsampling::CS422: chromaWidth /= 2; break;

            case Subsampling::CS444:
            case Subsampling::None: break;
            }

            return QSize(chromaWidth, chromaHeight);
        }
    }

    return QSize();
}

size_t
ImageBuffer::planeByteSize(int plane) const
{
    Q_ASSERT(plane >= 0 && plane < planeCount() && "plane index out of range.");

    const QSize size = planeSize(plane);
    const size_t stride = planeStride(plane);

    return size_t(size.height()) * stride;
}

quint8*
ImageBuffer::planeData(int plane) const
{
    Q_ASSERT(!p->d.data.isEmpty() && "imagebuffer not allocated. Call allocate() before accessing data.");

    Q_ASSERT(plane >= 0 && plane < planeCount()
             && "planeData/planeStride: plane index out of range. Valid range is [0, planeCount()).");

    quint8* base = reinterpret_cast<quint8*>(p->d.data.data());

    switch (p->d.packing) {
    case Packing::Interleaved:
    case Packing::Packed: return base;

    case Packing::Planar: {
        size_t planeSize = size_t(p->d.dataWindow.width()) * size_t(p->d.dataWindow.height()) * p->d.format.size();
        return base + size_t(plane) * planeSize;
    }

    case Packing::BiPlanar: {
        if (plane == 0)
            return base;

        size_t yPlaneSize = size_t(p->d.dataWindow.width()) * size_t(p->d.dataWindow.height()) * p->d.format.size();
        return base + yPlaneSize;
    }
    }

    return nullptr;
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
ImageBuffer::isAllocated() const
{
    return !p->d.data.isEmpty();
}

bool
ImageBuffer::isValid() const
{
    return p->d.format.isValid();
}

void
ImageBuffer::reset()
{
    detach();
    p->d = ImageBufferPrivate::Data();
}

ImageBuffer&
ImageBuffer::operator=(const ImageBuffer& other)
{
    if (this != &other)
        p = other.p;

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
ImageBuffer::convert(const ImageBuffer& source, ImageFormat::Type type, int channels)
{
    return detail::convertImage(source, type, channels);
}

ImageBuffer
ImageBuffer::convert(const ImageBuffer& source, int channels)
{
    return detail::convertChannels(source, channels);
}

}  // namespace flipman::sdk::core
