// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/imagebuffer.h>
#include <OpenImageIO/half.h>

#if defined(__ARM_NEON)
#    include <arm_neon.h>
#endif

#include <cstring>
#include <limits>

namespace flipman::sdk::core {

template<typename T> struct BigEnoughFloat {
    using type = float;
};
template<> struct BigEnoughFloat<int> {
    using type = double;
};
template<> struct BigEnoughFloat<unsigned> {
    using type = double;
};
template<> struct BigEnoughFloat<int64_t> {
    using type = double;
};
template<> struct BigEnoughFloat<uint64_t> {
    using type = double;
};
template<> struct BigEnoughFloat<half> {
    using type = float;
};
template<> struct BigEnoughFloat<double> {
    using type = double;
};

template<typename S, typename D, typename F>
inline D
scaledConversion(const S& src, F scale, F min, F max)
{
    if constexpr (std::numeric_limits<S>::is_signed) {
        F s = static_cast<F>(src) * scale;
        s += (s < F(0)) ? F(-0.5) : F(0.5);
        return static_cast<D>(std::clamp(s, min, max));
    }
    else {
        return static_cast<D>(std::clamp(static_cast<F>(src) * scale + F(0.5), min, max));
    }
}

template<typename S, typename D>
inline D
convertScalar(const S& src)
{
    if constexpr (std::is_same_v<S, D>)
        return src;

    using F = typename BigEnoughFloat<D>::type;

    F scale = std::numeric_limits<S>::is_integer ? (F(1) / F(std::numeric_limits<S>::max())) : F(1);

    if constexpr (std::numeric_limits<D>::is_integer) {
        F min = static_cast<F>(std::numeric_limits<D>::min());
        F max = static_cast<F>(std::numeric_limits<D>::max());
        scale *= max;
        return scaledConversion<S, D, F>(src, scale, min, max);
    }
    else {
        return static_cast<D>(static_cast<F>(src) * scale);
    }
}

template<typename S, typename D>
inline void
convertBuffer(const S* src, D* dst, size_t count)
{
    if constexpr (std::is_same_v<S, D>) {
        std::memcpy(dst, src, count * sizeof(D));
        return;
    }

    for (size_t i = 0; i < count; ++i)
        dst[i] = convertScalar<S, D>(src[i]);
}

template<typename F>
inline void
dispatchByFormat(ImageFormat::Type t, F&& fn)
{
    using T = ImageFormat::Type;

    switch (t) {
    case T::UInt8: fn(uint8_t {}); break;
    case T::Int8: fn(int8_t {}); break;
    case T::UInt16: fn(uint16_t {}); break;
    case T::Int16: fn(int16_t {}); break;
    case T::UInt32: fn(uint32_t {}); break;
    case T::Int32: fn(int32_t {}); break;
    case T::UInt64: fn(uint64_t {}); break;
    case T::Int64: fn(int64_t {}); break;
    case T::Half: fn(half {}); break;
    case T::Float: fn(float {}); break;
    case T::Double: fn(double {}); break;
    default: Q_ASSERT(false && "Unsupported ImageFormat::Type");
    }
}

class ImageBufferPrivate : public QSharedData {
public:
    ImageBufferPrivate();
    ~ImageBufferPrivate();
    void alloc();
    size_t byteSize() const;
    size_t pixelSize() const;
    size_t strideSize() const;
    size_t size() const;
    static ImageBuffer::PixelLayout pixelLayout(ImageBuffer::PixelLayout layout, int channels);
    static void convert(const ImageFormat& fromformat, const quint8* from, const ImageFormat& toformat, quint8* to,
                        int count);
    struct Data {
        ImageFormat format;
        QRect dataWindow;
        QRect displayWindow;
        int channels = 0;
        ImageBuffer::Packing packing = ImageBuffer::Packing::Interleaved;
        ImageBuffer::Subsampling subsampling = ImageBuffer::Subsampling::None;
        ImageBuffer::PixelLayout pixelLayout = ImageBuffer::PixelLayout::Unknown;
        ImageBuffer::PixelRange pixelRange = ImageBuffer::PixelRange::Unknown;
        render::ColorSpace colorSpace = render::ColorSpace::Unknown;
        render::TransferFunction transferFunction = render::TransferFunction::Unknown;
        QByteArray data;
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

ImageBuffer::PixelLayout
ImageBufferPrivate::pixelLayout(ImageBuffer::PixelLayout layout, int channels)
{
    switch (layout) {
    case ImageBuffer::PixelLayout::RGB:
    case ImageBuffer::PixelLayout::RGBA:
        return channels == 4 ? ImageBuffer::PixelLayout::RGBA : ImageBuffer::PixelLayout::RGB;

    case ImageBuffer::PixelLayout::BGR:
    case ImageBuffer::PixelLayout::BGRA:
        return channels == 4 ? ImageBuffer::PixelLayout::BGRA : ImageBuffer::PixelLayout::BGR;

    default: return layout;
    }
}

void
ImageBufferPrivate::convert(const ImageFormat& fromformat, const quint8* from, const ImageFormat& toformat, quint8* to,
                            int count)
{
    Q_ASSERT(from && "source pointer must not be null, must provide at least 'count' elements of fromformat.");
    Q_ASSERT(
        to && "Destination pointer must not be null, must provide storage for at least 'count' elements of toformat.");
    Q_ASSERT(count >= 0 && "count must be non-negative, count represents number of scalar components, not pixels.");

    if (fromformat.type() == toformat.type()) {
        std::memcpy(to, from, size_t(count) * fromformat.size());
        return;
    }

    dispatchByFormat(fromformat.type(), [&](auto sTag) {
        using S = decltype(sTag);
        const S* src = reinterpret_cast<const S*>(from);

        dispatchByFormat(toformat.type(), [&](auto dTag) {
            using D = decltype(dTag);
            D* dst = reinterpret_cast<D*>(to);

            convertBuffer<S, D>(src, dst, size_t(count));
        });
    });
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
ImageBuffer::convert(const ImageBuffer& imagebuffer, ImageFormat::Type type, int channels)
{
    Q_ASSERT(!imagebuffer.requiresDecode()
             && "imagebuffer::convert only supports native RGB-like interleaved images. "
                "YCbCr, planar, biplanar, and packed formats must be decoded first.");

    Q_ASSERT(imagebuffer.isRgb() && "ImageBuffer::convert only supports RGB-like image layouts.");

    Q_ASSERT(imagebuffer.p->d.packing == Packing::Interleaved
             && "imagebuffer::convert currently only supports interleaved packing.");

    Q_ASSERT(imagebuffer.p->d.subsampling == Subsampling::None
             && "imagebuffer::convert does not support chroma subsampled images.");

    if (imagebuffer.imageFormat().type() == type && imagebuffer.channels() == channels) {
        ImageBuffer copy = imagebuffer;
        return copy.detach();
    }

    ImageBuffer copy(imagebuffer.dataWindow(), imagebuffer.displayWindow(), type, channels);
    copy.setPacking(imagebuffer.packing());
    copy.setSubsampling(imagebuffer.subsampling());
    copy.setPixelLayout(ImageBufferPrivate::pixelLayout(imagebuffer.pixelLayout(), channels));
    copy.setPixelRange(imagebuffer.pixelRange());
    copy.setColorSpace(imagebuffer.colorSpace());
    copy.setTransferFunction(imagebuffer.transferFunction());
    copy.allocate();

    const quint8* src = imagebuffer.data();
    quint8* dst = copy.data();

    for (int y = 0; y < imagebuffer.dataWindow().height(); ++y) {
        const quint8* from = src + size_t(y) * imagebuffer.strideSize();
        quint8* to = dst + size_t(y) * copy.strideSize();

        for (int x = 0; x < imagebuffer.dataWindow().width(); ++x) {
            const int copyChannels = std::min(imagebuffer.channels(), copy.channels());

            ImageBufferPrivate::convert(imagebuffer.imageFormat(), from, copy.imageFormat(), to, copyChannels);

            if (copy.channels() > imagebuffer.channels()) {
                dispatchByFormat(copy.imageFormat().type(), [&](auto tag) {
                    using D = decltype(tag);
                    D* dstPixel = reinterpret_cast<D*>(to);

                    for (int c = copyChannels; c < copy.channels(); ++c)
                        dstPixel[c] = std::numeric_limits<D>::is_integer ? std::numeric_limits<D>::max() : D(1);
                });
            }

            from += imagebuffer.pixelSize();
            to += copy.pixelSize();
        }
    }

    return copy;
}

ImageBuffer
ImageBuffer::convert(const ImageBuffer& imageBuffer, int channels)
{
    Q_ASSERT(!imageBuffer.requiresDecode()
             && "imagebuffer::convert only supports native RGB-like interleaved images. "
                "YCbCr, planar, biplanar, and packed formats must be decoded first.");

    Q_ASSERT(imageBuffer.isRgb() && "ImageBuffer::convert only supports RGB-like image layouts.");

    Q_ASSERT(imageBuffer.p->d.packing == Packing::Interleaved
             && "imagebuffer::convert currently only supports interleaved packing.");

    Q_ASSERT(imageBuffer.p->d.subsampling == Subsampling::None
             && "imagebuffer::convert does not support chroma subsampled images.");

    if (imageBuffer.channels() == channels)
        return imageBuffer;

    const int width = imageBuffer.dataWindow().width();
    const int height = imageBuffer.dataWindow().height();

    ImageBuffer dst(imageBuffer.dataWindow(), imageBuffer.displayWindow(), imageBuffer.imageFormat(), channels);
    dst.setPacking(imageBuffer.packing());
    dst.setSubsampling(imageBuffer.subsampling());
    dst.setPixelLayout(ImageBufferPrivate::pixelLayout(imageBuffer.pixelLayout(), channels));
    dst.setPixelRange(imageBuffer.pixelRange());
    dst.setColorSpace(imageBuffer.colorSpace());
    dst.setTransferFunction(imageBuffer.transferFunction());
    dst.allocate();

    const int srcChannels = imageBuffer.channels();
    const auto type = imageBuffer.imageFormat().type();

    const quint8* srcBase = imageBuffer.data();
    quint8* dstBase = dst.data();

    if (srcChannels == 3 && channels == 4) {
        if (type == ImageFormat::UInt8) {
#if defined(__ARM_NEON)
            for (int y = 0; y < height; ++y) {
                const uint8_t* s = reinterpret_cast<const uint8_t*>(srcBase + y * imageBuffer.strideSize());
                uint8_t* d = reinterpret_cast<uint8_t*>(dstBase + y * dst.strideSize());

                int x = 0;
                const uint8x16_t alpha = vdupq_n_u8(255);

                for (; x + 16 <= width; x += 16) {
                    uint8x16x3_t rgb = vld3q_u8(s);
                    uint8x16x4_t rgba;
                    rgba.val[0] = rgb.val[0];
                    rgba.val[1] = rgb.val[1];
                    rgba.val[2] = rgb.val[2];
                    rgba.val[3] = alpha;
                    vst4q_u8(d, rgba);

                    s += 16 * 3;
                    d += 16 * 4;
                }

                for (; x < width; ++x) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    d[3] = 255;
                    s += 3;
                    d += 4;
                }
            }
#else
            for (int y = 0; y < height; ++y) {
                const uint8_t* s = reinterpret_cast<const uint8_t*>(srcBase + y * imageBuffer.strideSize());
                uint8_t* d = reinterpret_cast<uint8_t*>(dstBase + y * dst.strideSize());

                for (int x = 0; x < width; ++x) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    d[3] = 255;
                    s += 3;
                    d += 4;
                }
            }
#endif
            return dst;
        }

        if (type == ImageFormat::Half) {
            static_assert(sizeof(half) == sizeof(uint16_t), "OIIO::half must be 16-bit");

            const half alphaHalf = half(1.0f);
            uint16_t alphaBits = 0;
            std::memcpy(&alphaBits, &alphaHalf, sizeof(alphaBits));

#if defined(__ARM_NEON)
            for (int y = 0; y < height; ++y) {
                const uint16_t* s = reinterpret_cast<const uint16_t*>(srcBase + y * imageBuffer.strideSize());
                uint16_t* d = reinterpret_cast<uint16_t*>(dstBase + y * dst.strideSize());

                int x = 0;
                const uint16x8_t alpha = vdupq_n_u16(alphaBits);

                for (; x + 8 <= width; x += 8) {
                    uint16x8x3_t rgb = vld3q_u16(s);
                    uint16x8x4_t rgba;
                    rgba.val[0] = rgb.val[0];
                    rgba.val[1] = rgb.val[1];
                    rgba.val[2] = rgb.val[2];
                    rgba.val[3] = alpha;
                    vst4q_u16(d, rgba);

                    s += 8 * 3;
                    d += 8 * 4;
                }

                for (; x < width; ++x) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    d[3] = alphaBits;
                    s += 3;
                    d += 4;
                }
            }
#else
            for (int y = 0; y < height; ++y) {
                const half* s = reinterpret_cast<const half*>(srcBase + y * imageBuffer.strideSize());
                half* d = reinterpret_cast<half*>(dstBase + y * dst.strideSize());

                for (int x = 0; x < width; ++x) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    d[3] = alphaHalf;
                    s += 3;
                    d += 4;
                }
            }
#endif
            return dst;
        }

        if (type == ImageFormat::Float) {
#if defined(__ARM_NEON)
            for (int y = 0; y < height; ++y) {
                const float* s = reinterpret_cast<const float*>(srcBase + y * imageBuffer.strideSize());
                float* d = reinterpret_cast<float*>(dstBase + y * dst.strideSize());

                int x = 0;
                const float32x4_t alpha = vdupq_n_f32(1.0f);

                for (; x + 4 <= width; x += 4) {
                    float32x4x3_t rgb = vld3q_f32(s);
                    float32x4x4_t rgba;
                    rgba.val[0] = rgb.val[0];
                    rgba.val[1] = rgb.val[1];
                    rgba.val[2] = rgb.val[2];
                    rgba.val[3] = alpha;
                    vst4q_f32(d, rgba);

                    s += 4 * 3;
                    d += 4 * 4;
                }

                for (; x < width; ++x) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    d[3] = 1.0f;
                    s += 3;
                    d += 4;
                }
            }
#else
            for (int y = 0; y < height; ++y) {
                const float* s = reinterpret_cast<const float*>(srcBase + y * imageBuffer.strideSize());
                float* d = reinterpret_cast<float*>(dstBase + y * dst.strideSize());

                for (int x = 0; x < width; ++x) {
                    d[0] = s[0];
                    d[1] = s[1];
                    d[2] = s[2];
                    d[3] = 1.0f;
                    s += 3;
                    d += 4;
                }
            }
#endif
            return dst;
        }
    }

    if (type == ImageFormat::UInt8) {
        for (int y = 0; y < height; ++y) {
            const uint8_t* s = reinterpret_cast<const uint8_t*>(srcBase + y * imageBuffer.strideSize());
            uint8_t* d = reinterpret_cast<uint8_t*>(dstBase + y * dst.strideSize());

            for (int x = 0; x < width; ++x) {
                for (int c = 0; c < std::min(srcChannels, channels); ++c)
                    d[c] = s[c];

                for (int c = srcChannels; c < channels; ++c)
                    d[c] = 255;

                s += srcChannels;
                d += channels;
            }
        }
        return dst;
    }

    if (type == ImageFormat::Half) {
        const half alpha = half(1.0f);
        for (int y = 0; y < height; ++y) {
            const half* s = reinterpret_cast<const half*>(srcBase + y * imageBuffer.strideSize());
            half* d = reinterpret_cast<half*>(dstBase + y * dst.strideSize());

            for (int x = 0; x < width; ++x) {
                for (int c = 0; c < std::min(srcChannels, channels); ++c)
                    d[c] = s[c];

                for (int c = srcChannels; c < channels; ++c)
                    d[c] = alpha;

                s += srcChannels;
                d += channels;
            }
        }
        return dst;
    }

    if (type == ImageFormat::Float) {
        const float alpha = 1.0f;
        for (int y = 0; y < height; ++y) {
            const float* s = reinterpret_cast<const float*>(srcBase + y * imageBuffer.strideSize());
            float* d = reinterpret_cast<float*>(dstBase + y * dst.strideSize());

            for (int x = 0; x < width; ++x) {
                for (int c = 0; c < std::min(srcChannels, channels); ++c)
                    d[c] = s[c];

                for (int c = srcChannels; c < channels; ++c)
                    d[c] = alpha;

                s += srcChannels;
                d += channels;
            }
        }
        return dst;
    }
    Q_ASSERT(false && "ImageBuffer::convert unsupported image format");
    return {};
}

}  // namespace flipman::sdk::core
