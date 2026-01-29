// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/imageformat.h>

namespace flipman::sdk::core {
class ImageFormatPrivate : public QSharedData {
public:
    ImageFormatPrivate();
    ~ImageFormatPrivate();
    struct Data {
        int typesize[ImageFormat::DOUBLE + 1];
        ImageFormat::Type type;
        Data()
            : typesize {
                0,
                sizeof(quint8),
                sizeof(qint8),
                sizeof(quint16),
                sizeof(qint16),
                sizeof(quint32),
                sizeof(qint32),
                sizeof(quint64),
                sizeof(qint64),
                sizeof(float) / 2,
                sizeof(float),
                sizeof(double),
            }
        {}
    };
    Data d;
};

ImageFormatPrivate::ImageFormatPrivate() {}

ImageFormatPrivate::~ImageFormatPrivate() {}

ImageFormat::ImageFormat()
    : p(new ImageFormatPrivate())
{}

ImageFormat::ImageFormat(ImageFormat::Type type)
    : p(new ImageFormatPrivate())
{
    p->d.type = type;
}

ImageFormat::ImageFormat(const ImageFormat& other)
    : p(other.p)
{}

ImageFormat::~ImageFormat() {}

size_t
ImageFormat::size() const
{
    Q_ASSERT("type index is out of bounds" && p->d.type >= 0 && p->d.type < ImageFormat::DOUBLE + 1);
    return p->d.typesize[p->d.type];
}

ImageFormat::Type
ImageFormat::type() const
{
    return p->d.type;
}

bool
ImageFormat::isValid() const
{
    return p->d.type > 0;
}

void
ImageFormat::reset()
{
    p.detach();
    p->d.type = ImageFormat::NONE;
}

ImageFormat&
ImageFormat::operator=(const ImageFormat& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
ImageFormat::operator==(const ImageFormat& other) const
{
    return p->d.type == other.p->d.type;
}

bool
ImageFormat::operator!=(const ImageFormat& other) const
{
    return !(*this == other);
}

bool
ImageFormat::operator<(const ImageFormat& other) const
{
    return (p->d.type < other.p->d.type);
}

bool
ImageFormat::operator>(const ImageFormat& other) const
{
    return (p->d.type > other.p->d.type);
}
}  // namespace flipman::sdk::core
