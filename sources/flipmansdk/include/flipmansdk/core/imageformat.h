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
 * @brief Explicitly shared pixel format descriptor.
 */
class FLIPMANSDK_EXPORT ImageFormat {
    Q_GADGET
public:
    /**
     * @brief Supported pixel component types.
     */
    enum Type { None, UInt8, Int8, UInt16, Int16, UInt32, Int32, UInt64, Int64, Half, Float, Double };
    Q_ENUM(Type)

    /**
     * @brief Constructs an invalid ImageFormat.
     */
    ImageFormat();

    /**
     * @brief Constructs an ImageFormat from a Type.
     */
    ImageFormat(Type type);

    /**
     * @brief Copy constructor.
     */
    ImageFormat(const ImageFormat& other);

    /**
     * @brief Destroys the ImageFormat.
     */
    ~ImageFormat();

    /**
     * @brief Returns the byte size of a single component.
     */
    size_t size() const;

    /**
     * @brief Returns the pixel type.
     */
    Type type() const;

    /**
     * @brief Returns true if valid.
     */
    bool isValid() const;

    /**
     * @brief Resets to invalid state.
     */
    void reset();

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    ImageFormat& operator=(const ImageFormat& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const ImageFormat& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const ImageFormat& other) const;

    /**
     * @brief Strict ordering operator.
     */
    bool operator<(const ImageFormat& other) const;

    /**
     * @brief Greater-than operator.
     */
    bool operator>(const ImageFormat& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<ImageFormatPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::ImageFormat)
