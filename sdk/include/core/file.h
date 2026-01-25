// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman
#pragma once

#include <core/object.h>

#include <QtCore/QFileInfo>
#include <QtCore/QObject>

namespace core {
class FilePrivate;
class FileRange;
class File : public Object {
public:
    File();
    File(const File& file);
    File(const QString& file);
    File(const QFileInfo& info);
    bool is_valid() const override;
    virtual ~File();
    QString absolutepath() const;
    QString dirname() const;
    QString basename() const;
    QString filename() const;
    QString filename(qint64 frame) const;
    QString filepath() const;
    QString extension() const;
    QString display_name() const;
    QString display_size() const;
    qint64 size() const;
    QString owner() const;
    QString group() const;
    QDateTime created() const;
    QDateTime modified() const;
    bool exists() const;
    bool is_readable() const;
    bool is_writable() const;
    bool is_executable() const;
    FileRange filerange() const;
    void reset() override;

    void set_filerange(const FileRange& range);

    File& operator=(const File& other);
    bool operator==(const File& other) const;
    bool operator!=(const File& other) const;
    operator QString() const;

    static QList<File> list(const QString& filepath, const QStringList& namefilters = QStringList() << "*",
                            bool ranges = true);

private:
    QExplicitlySharedDataPointer<FilePrivate> p;
};
}  // namespace core
