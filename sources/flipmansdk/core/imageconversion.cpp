// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "private/imageconversion.h"

#include <flipmansdk/core/perfscope.h>
#include <flipmansdk/core/profile.h>
#include <OpenImageIO/half.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>

#if defined(__ARM_NEON)
#    include <arm_neon.h>
#endif

namespace flipman::sdk::core::detail {

namespace {

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

    template<typename S, typename D, typename F> inline D scaledConversion(const S& src, F scale, F min, F max)
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

    template<typename S, typename D> inline D convertScalar(const S& src)
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

    template<typename S, typename D> inline void convertBuffer(const S* src, D* dst, size_t count)
    {
        if constexpr (std::is_same_v<S, D>) {
            std::memcpy(dst, src, count * sizeof(D));
            return;
        }

        for (size_t i = 0; i < count; ++i)
            dst[i] = convertScalar<S, D>(src[i]);
    }

    template<typename F> inline void dispatchByFormat(ImageFormat::Type t, F&& fn)
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

    ImageBuffer::PixelLayout convertedPixelLayout(ImageBuffer::PixelLayout layout, int channels)
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

    void convertComponents(const ImageFormat& fromformat, const quint8* from, const ImageFormat& toformat, quint8* to,
                           int count)
    {
        Q_ASSERT(from && "source pointer must not be null, must provide at least 'count' elements of fromformat.");
        Q_ASSERT(
            to
            && "Destination pointer must not be null, must provide storage for at least 'count' elements of toformat.");
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

}  // namespace

ImageBuffer
convertImage(const ImageBuffer& imagebuffer, ImageFormat::Type type, int channels)
{
    FM_PROFILE_SCOPE("image", "convert_format");
    PerfScope phase("image.convert_format");
    Q_ASSERT(!imagebuffer.requiresDecode()
             && "imagebuffer::convert only supports native RGB-like interleaved images. "
                "YCbCr, planar, biplanar, and packed formats must be decoded first.");

    Q_ASSERT(imagebuffer.isRgb() && "ImageBuffer::convert only supports RGB-like image layouts.");

    Q_ASSERT(imagebuffer.packing() == ImageBuffer::Packing::Interleaved
             && "imagebuffer::convert currently only supports interleaved packing.");

    Q_ASSERT(imagebuffer.subsampling() == ImageBuffer::Subsampling::None
             && "imagebuffer::convert does not support chroma subsampled images.");

    if (imagebuffer.imageFormat().type() == type && imagebuffer.channels() == channels) {
        ImageBuffer copy = imagebuffer;
        return copy.detach();
    }

    ImageBuffer copy(imagebuffer.dataWindow(), imagebuffer.displayWindow(), type, channels);
    copy.setPacking(imagebuffer.packing());
    copy.setSubsampling(imagebuffer.subsampling());
    copy.setPixelLayout(convertedPixelLayout(imagebuffer.pixelLayout(), channels));
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

            convertComponents(imagebuffer.imageFormat(), from, copy.imageFormat(), to, copyChannels);

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
convertChannels(const ImageBuffer& imageBuffer, int channels)
{
    FM_PROFILE_SCOPE("image", "convert_channels");
    PerfScope phase("image.convert_channels");
    Q_ASSERT(!imageBuffer.requiresDecode()
             && "imagebuffer::convert only supports native RGB-like interleaved images. "
                "YCbCr, planar, biplanar, and packed formats must be decoded first.");

    Q_ASSERT(imageBuffer.isRgb() && "ImageBuffer::convert only supports RGB-like image layouts.");

    Q_ASSERT(imageBuffer.packing() == ImageBuffer::Packing::Interleaved
             && "imagebuffer::convert currently only supports interleaved packing.");

    Q_ASSERT(imageBuffer.subsampling() == ImageBuffer::Subsampling::None
             && "imagebuffer::convert does not support chroma subsampled images.");

    if (imageBuffer.channels() == channels)
        return imageBuffer;

    const int width = imageBuffer.dataWindow().width();
    const int height = imageBuffer.dataWindow().height();

    ImageBuffer dst(imageBuffer.dataWindow(), imageBuffer.displayWindow(), imageBuffer.imageFormat(), channels);
    dst.setPacking(imageBuffer.packing());
    dst.setSubsampling(imageBuffer.subsampling());
    dst.setPixelLayout(convertedPixelLayout(imageBuffer.pixelLayout(), channels));
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

}  // namespace flipman::sdk::core::detail
