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
 *
 * This class uses explicit sharing to ensure lightweight copying and is
 * registered with the Qt Meta-Object system via Q_GADGET to support
 * enum-to-string conversion and metadata inspection.
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

    /**
     * @brief Constructs an invalid image format (NONE).
     */
    ImageFormat();

    /**
     * @brief Constructs a format with the specified pixel type.
     * @param type The pixel bit-depth or floating-point format.
     */
    ImageFormat(ImageFormat::Type type);

    /**
     * @brief Copy constructor. Performs a shallow copy of the shared data.
     */
    ImageFormat(const ImageFormat& other);

    /**
     * @brief Destroys the image format.
     * @note Required for the PIMPL pattern to safely delete ImageFormatPrivate.
     */
    ~ImageFormat();

    /**
     * @brief Returns the byte size of a single pixel component for the current type.
     * @return Size in bytes (e.g., 4 for FLOAT, 1 for UINT8).
     */
    size_t size() const;

    /**
     * @brief Returns the current pixel type.
     */
    ImageFormat::Type type() const;

    /**
     * @brief Returns true if the type is not NONE.
     */
    bool isValid() const;

    /**
     * @brief Resets the format to NONE.
     */
    void reset();

    /** @name Operators */
    ///@{
    ImageFormat& operator=(const ImageFormat& other);
    bool operator==(const ImageFormat& other) const;
    bool operator!=(const ImageFormat& other) const;
    bool operator<(const ImageFormat& other) const;
    bool operator>(const ImageFormat& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<ImageFormatPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::ImageFormat)
