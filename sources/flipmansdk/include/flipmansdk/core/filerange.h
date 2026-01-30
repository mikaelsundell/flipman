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
 * @brief Manages a sequence of files mapped to specific frame numbers.
 *
 * FileRange is used to handle image sequences or frame-based assets. It tracks
 * the relationship between a linear timeline (frames) and specific file paths
 * on disk using explicit data sharing for performance.
 */
class FLIPMANSDK_EXPORT FileRange {
public:
    /**
     * @brief Constructs an empty file range.
     */
    FileRange();

    /**
     * @brief Copy constructor. Performs a shallow copy of the shared data.
     */
    FileRange(const FileRange& other);

    /**
     * @brief Destroys the file range.
     * @note Required for the PIMPL pattern to safely delete FileRangePrivate.
     */
    ~FileRange();

    /** @name Frame Access */
    ///@{
    /**
     * @brief Returns true if a specific frame number is mapped to a file.
     */
    bool hasFrame(qint64 frame) const;

    /**
     * @brief Returns the File object associated with the given frame number.
     */
    File frame(qint64 frame) const;

    /**
     * @brief Returns the first frame number in the sequence.
     */
    qint64 start() const;

    /**
     * @brief Returns the last frame number in the sequence.
     */
    qint64 end() const;

    /**
     * @brief Returns the total number of frames in the sequence.
     */
    qint64 size() const;
    ///@}

    /** @name Management */
    ///@{
    /**
     * @brief Returns true if the range contains at least one valid frame mapping.
     */
    bool isValid() const;

    /**
     * @brief Clears all frame mappings and resets the range.
     */
    void reset();

    /**
     * @brief Maps a specific frame number to a file.
     * @param frame The index of the frame.
     * @param file The file associated with this frame.
     */
    void insertFrame(qint64 frame, const File& file);
    ///@}

    /** @name Operators */
    ///@{
    FileRange& operator=(const FileRange& other);
    bool operator==(const FileRange& other) const;
    bool operator!=(const FileRange& other) const;
    bool operator<(const FileRange& other) const;
    bool operator>(const FileRange& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<FileRangePrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::FileRange)
