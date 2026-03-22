// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QtCore/QDateTime>
#include <QtCore/QExplicitlySharedDataPointer>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaType>

namespace flipman::sdk::core {

class FilePrivate;
class FileRange;

/**
 * @class File
 * @brief Explicitly shared file and sequence abstraction.
 */
class FLIPMANSDK_EXPORT File {
public:
    /**
     * @brief Constructs an empty File.
     */
    File();

    /**
     * @brief Copy constructor.
     */
    File(const File& other);

    /**
     * @brief Constructs a File from a path.
     */
    explicit File(const QString& file);

    /**
     * @brief Constructs a File from QFileInfo.
     */
    File(const QFileInfo& info);

    /**
     * @brief Destroys the File.
     */
    ~File();

    /** @name Path Information */
    ///@{

    /**
     * @brief Returns the absolute file path.
     */
    QString absolutePath() const;

    /**
     * @brief Returns the directory path.
     */
    QString dirName() const;

    /**
     * @brief Returns the base name without extension.
     */
    QString baseName() const;

    /**
     * @brief Returns the file name including extension.
     */
    QString fileName() const;

    /**
     * @brief Returns a resolved file name for a specific frame.
     */
    QString fileName(qint64 frame) const;

    /**
     * @brief Returns the original file path.
     */
    QString filePath() const;

    /**
     * @brief Returns the file extension.
     */
    QString extension() const;

    ///@}

    /** @name Metadata */
    ///@{

    /**
     * @brief Returns display name.
     */
    QString displayName() const;

    /**
     * @brief Returns formatted file size.
     */
    QString displaySize() const;

    /**
     * @brief Returns file size in bytes.
     */
    qint64 size() const;

    /**
     * @brief Returns file owner.
     */
    QString owner() const;

    /**
     * @brief Returns file group.
     */
    QString group() const;

    /**
     * @brief Returns creation timestamp.
     */
    QDateTime created() const;

    /**
     * @brief Returns modification timestamp.
     */
    QDateTime modified() const;

    ///@}

    /** @name State */
    ///@{

    /**
     * @brief Returns true if the file exists.
     */
    bool exists() const;

    /**
     * @brief Returns true if readable.
     */
    bool isReadable() const;

    /**
     * @brief Returns true if writable.
     */
    bool isWritable() const;

    /**
     * @brief Returns true if executable.
     */
    bool isExecutable() const;

    /**
     * @brief Returns true if the file path is valid.
     */
    bool isValid() const;

    /**
     * @brief Resets to an empty state.
     */
    void reset();

    ///@}

    /** @name Sequence */
    ///@{

    /**
     * @brief Returns associated FileRange.
     */
    FileRange fileRange() const;

    /**
     * @brief Sets associated FileRange.
     */
    void setFileRange(const FileRange& range);

    ///@}

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    File& operator=(const File& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const File& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const File& other) const;

    /**
     * @brief Conversion operator returning file path.
     */
    operator QString() const;

    ///@}

    /**
     * @brief Lists files in a directory.
     *
     * @param filepath Directory path.
     * @param namefilters Optional glob filters.
     * @param ranges Enables automatic sequence grouping.
     *
     * @return List of File objects.
     */
    static QList<File> listDir(const QString& filepath, const QStringList& namefilters = QStringList() << "*",
                               bool ranges = true);

private:
    QExplicitlySharedDataPointer<FilePrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::File)
