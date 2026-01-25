// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/file.h>
#include <core/object.h>

#include <QtCore/QObject>

namespace core {
class FileRangePrivate;
class FileRange : public Object {
public:
    FileRange();
    FileRange(const FileRange& filerange);
    virtual ~FileRange();
    bool has_frame(qint64 frame) const;
    File frame(qint64 frame) const;
    qint64 start() const;
    qint64 end() const;
    qint64 size() const;
    void reset();
    bool is_valid() const;

    void insert_frame(qint64 frame, const File& file);

    FileRange& operator=(const FileRange& filerange);
    bool operator==(const FileRange& filerange) const;
    bool operator!=(const FileRange& filerange) const;
    bool operator<(const FileRange& filerange) const;
    bool operator>(const FileRange& filerange) const;

private:
    QExplicitlySharedDataPointer<FileRangePrivate> p;
};
}  // namespace core
