
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/object.h>

#include <QExplicitlySharedDataPointer>
#include <QString>

namespace core {
class ImageFormatPrivate;
class ImageFormat : public Object {
public:
    enum Type { NONE, UINT8, INT8, UINT16, INT16, UINT32, INT32, UINT64, INT64, HALF, FLOAT, DOUBLE };
    ImageFormat();
    ImageFormat(ImageFormat::Type type);
    ImageFormat(const ImageFormat& other);
    virtual ~ImageFormat();
    size_t size() const;
    ImageFormat::Type type() const;
    bool is_valid() const;
    void reset();

    ImageFormat& operator=(const ImageFormat& other);
    bool operator==(const ImageFormat& other) const;
    bool operator!=(const ImageFormat& other) const;
    bool operator<(const ImageFormat& other) const;
    bool operator>(const ImageFormat& other) const;

private:
    QExplicitlySharedDataPointer<ImageFormatPrivate> p;
};
}  // namespace core
