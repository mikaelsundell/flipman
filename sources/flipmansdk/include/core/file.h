// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QObject>

namespace flipman::sdk::core {

class FilePrivate;
class FileRange;

/**
 * @class File
 * @brief High-level file and sequence abstraction with explicit data sharing.
 * * Extends basic file information with features specific to media workflows,
 * such as frame-pattern resolution, sequence range awareness, and formatted
 * display attributes for UI components.
 */
class FLIPMANSDK_EXPORT File {
public:
    /// Constructs an empty file object.
    File();

    /// Copy constructor (shallow copy).
    File(const File& file);

    /// Constructs a file object from a string path.
    File(const QString& file);

    /// Constructs a file object from existing QFileInfo.
    File(const QFileInfo& info);

    virtual ~File();

    /// Returns the absolute path including the filename.
    QString absolutePath() const;

    /// Returns the directory path (excluding the filename).
    QString dirName() const;

    /// Returns the base name of the file without the path or extension.
    QString baseName() const;

    /// Returns the filename including the extension.
    QString fileName() const;

    /**
     * @brief Returns a resolved filename for a specific frame.
     * Useful for resolving patterns like "image.####.exr" to "image.1001.exr".
     */
    QString fileName(qint64 frame) const;

    /// Returns the full path as provided during construction.
    QString filePath() const;

    /// Returns the file extension (e.g., "exr", "jpg").
    QString extension() const;

    /// Returns a localized or "friendly" name for UI display.
    QString displayName() const;

    /// Returns a human-readable string of the file size (e.g., "1.2 GB").
    QString displaySize() const;

    /// Returns the raw file size in bytes.
    qint64 size() const;

    /// Returns the name of the file owner.
    QString owner() const;

    /// Returns the name of the file group.
    QString group() const;

    /// Returns the file creation date and time.
    QDateTime created() const;

    /// Returns the last modification date and time.
    QDateTime modified() const;

    /// Returns true if the file exists on the physical file system.
    bool exists() const;

    /// Returns true if the current user has read permissions.
    bool isReadable() const;

    /// Returns true if the current user has write permissions.
    bool isWritable() const;

    /// Returns true if the file is an executable.
    bool isExecutable() const;

    /// Returns the associated FileRange if this file is part of a sequence.
    FileRange fileRange() const;

    /// Returns true if the object points to a valid file path.
    bool isValid() const;

    /// Resets the object to a null state.
    void reset();

    /// Associates a specific frame range with this file entry.
    void setFileRange(const FileRange& range);

    File& operator=(const File& other);
    bool operator==(const File& other) const;
    bool operator!=(const File& other) const;

    /// Convenience operator to return the file path as a QString.
    operator QString() const;

    /**
     * @brief Lists files in a directory with optional sequence detection.
     * @param filepath The directory to scan.
     * @param namefilters Glob patterns (e.g., "*.exr").
     * @param ranges If true, automatically groups individual files into FileRanges.
     * @return A list of File objects found.
     */
    static QList<File> listDir(const QString& filepath, const QStringList& namefilters = QStringList() << "*",
                               bool ranges = true);

private:
    QExplicitlySharedDataPointer<FilePrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::File)
