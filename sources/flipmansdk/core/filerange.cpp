// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QMap>
#include <core/filerange.h>

namespace flipman::sdk::core {
class FileRangePrivate : public QSharedData {
public:
    struct Data {
        QMap<qint64, File> ranges;
    };
    Data d;
};

FileRange::FileRange()
    : p(new FileRangePrivate())
{}

FileRange::FileRange(const FileRange& other)
    : p(other.p)
{}

FileRange::~FileRange() {}

bool
FileRange::hasFrame(qint64 frame) const
{
    return p->d.ranges.contains(frame);
}

File
FileRange::frame(qint64 frame) const
{
    Q_ASSERT("Frame not found in range" && hasFrame(frame));
    return (p->d.ranges.value(frame));
}

qint64
FileRange::start() const
{
    return p->d.ranges.keys().front();
}

qint64
FileRange::end() const
{
    return p->d.ranges.keys().back();
}

qint64
FileRange::size() const
{
    return p->d.ranges.size();
}

bool
FileRange::isValid() const
{
    return size();
}

void
FileRange::reset()
{
    p.detach();
    p->d.ranges.clear();
}

void
FileRange::insertFrame(qint64 frame, const File& file)
{
    p.detach();
    p->d.ranges.insert(frame, file);
}

FileRange&
FileRange::operator=(const FileRange& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
FileRange::operator==(const FileRange& filerange) const
{
    return p->d.ranges == filerange.p->d.ranges;
}

bool
FileRange::operator!=(const FileRange& filerange) const
{
    return !(*this == filerange);
}

bool
FileRange::operator<(const FileRange& filerange) const
{
    return size() < filerange.size();
}

bool
FileRange::operator>(const FileRange& filerange) const
{
    return size() > filerange.size();
}
}  // namespace flipman::sdk::core
