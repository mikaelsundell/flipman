// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/core/file.h>
#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::core {

class FileRangePrivate;

/**
 * @class FileRange
 * @brief Explicitly shared frame-to-file mapping.
 */
class FLIPMANSDK_EXPORT FileRange {
public:
    /**
     * @brief Constructs an empty FileRange.
     */
    FileRange();

    /**
     * @brief Copy constructor.
     */
    FileRange(const FileRange& other);

    /**
     * @brief Destroys the FileRange.
     */
    ~FileRange();

    /** @name Frame Access */
    ///@{

    /**
     * @brief Returns true if a frame is mapped.
     */
    bool hasFrame(qint64 frame) const;

    /**
     * @brief Returns the File mapped to a frame.
     */
    File frame(qint64 frame) const;

    /**
     * @brief Returns the first frame index.
     */
    qint64 start() const;

    /**
     * @brief Returns the last frame index.
     */
    qint64 end() const;

    /**
     * @brief Returns the number of mapped frames.
     */
    qint64 size() const;

    ///@}

    /** @name Management */
    ///@{

    /**
     * @brief Returns true if the range contains frames.
     */
    bool isValid() const;

    /**
     * @brief Resets the range to an empty state.
     */
    void reset();

    /**
     * @brief Inserts a frame-to-file mapping.
     */
    void insertFrame(qint64 frame, const File& file);

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    FileRange& operator=(const FileRange& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const FileRange& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const FileRange& other) const;

    /**
     * @brief Strict ordering operator.
     *
     * Provides deterministic ordering.
     */
    bool operator<(const FileRange& other) const;

    /**
     * @brief Greater-than operator.
     */
    bool operator>(const FileRange& other) const;

    ///@}

private:
    QExplicitlySharedDataPointer<FileRangePrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::FileRange)
