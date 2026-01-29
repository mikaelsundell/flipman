// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QObject>

namespace flipman::sdk::core {

class ImageFormatPrivate;

/**
 * @class ImageFormat
 * @brief Represents pixel data types and provides memory size information.
 * * This class uses explicit sharing to ensure lightweight copying and is
 * registered with the Qt Meta-Object system via Q_GADGET.
 */
class FLIPMANSDK_EXPORT ImageFormat {
    Q_GADGET
public:
    /**
     * @enum Type
     * @brief Supported pixel bit-depths and floating-point formats.
     */
    enum Type { NONE, UINT8, INT8, UINT16, INT16, UINT32, INT32, UINT64, INT64, HALF, FLOAT, DOUBLE };
    Q_ENUM(Type)

    /// Constructs an invalid image format.
    ImageFormat();

    /// Constructs a format with the specified pixel type.
    ImageFormat(ImageFormat::Type type);

    /// Copy constructor (shallow copy via shared data).
    ImageFormat(const ImageFormat& other);

    virtual ~ImageFormat();

    /// Returns the byte size of a single pixel for the current type.
    size_t size() const;

    /// Returns the current pixel type.
    ImageFormat::Type type() const;

    /// Returns true if the type is not NONE.
    bool isValid() const;

    /// Resets the format to NONE.
    void reset();

    ImageFormat& operator=(const ImageFormat& other);
    bool operator==(const ImageFormat& other) const;
    bool operator!=(const ImageFormat& other) const;
    bool operator<(const ImageFormat& other) const;
    bool operator>(const ImageFormat& other) const;

private:
    QExplicitlySharedDataPointer<ImageFormatPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::ImageFormat)
