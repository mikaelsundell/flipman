// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/file.h>
#include <flipmansdk/core/filerange.h>

#include <QDir>
#include <QRegularExpression>

namespace flipman::sdk::core {
class FilePrivate : public QSharedData {
public:
    struct FilePattern {
        QString filename;
        qint64 frame;
        qsizetype pos;
        qsizetype length;
    };
    static FilePattern pattern(const QString& filename);
    struct Data {
        QFileInfo fileInfo;
        FileRange fileRange;
        QChar divider = '#';
    };
    Data d;
};

FilePrivate::FilePattern
FilePrivate::pattern(const QString& filename)
{
    QRegularExpression regex("(\\d+)");
    QRegularExpressionMatchIterator matchIterator = regex.globalMatch(filename);
    QString cap;
    qsizetype pos = -1;
    qsizetype length = 0;
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        cap = match.captured(1);
        pos = match.capturedStart(1);
        length = cap.length();
    }
    if (pos != -1 && !cap.isEmpty()) {
        qint64 frame = cap.toLongLong();
        QString fill(length, '#');
        QString modifiedFilename = filename;
        modifiedFilename.replace(pos, length, fill);
        return FilePattern { modifiedFilename, frame, pos, length };
    }
    return FilePattern { filename, 0, -1, 0 };
}

File::File()
    : p(new FilePrivate())
{}

File::File(const File& other)
    : p(other.p)
{}

File::File(const QString& file)
    : p(new FilePrivate())
{
    if (file.contains(p->d.divider)) {
        QFileInfo info(file);
        QString path = info.absolutePath();
        QString filter = info.fileName().replace(QRegularExpression(QString("[%1]{1,}").arg(p->d.divider)), "*");
        QList<File> files = File::listDir(path, QStringList() << filter, true);
        if (files.size()) {
            p->d.fileInfo = files.first().p->d.fileInfo;
            p->d.fileRange = files.first().p->d.fileRange;
        }
        else {
            p->d.fileInfo = QFileInfo(file);
        }
    }
    else {
        p->d.fileInfo = QFileInfo(file);
    }
}

File::File(const QFileInfo& fileinfo)
    : p(new FilePrivate())
{
    p->d.fileInfo = fileinfo;
}

File::~File() {}

QString
File::displayName() const
{
    if (p->d.fileRange.size()) {
        FilePrivate::FilePattern pattern = FilePrivate::pattern(fileName());
        QString filename = pattern.filename;
        QString range = QString("[%1-%2]").arg(p->d.fileRange.start()).arg(p->d.fileRange.end());
        return filename.replace(pattern.pos, pattern.length, range);
    }
    else {
        return fileName();
    }
}

QString
File::displaySize() const
{
    qint64 bytes = p->d.fileInfo.size();
    const qint64 kb = 1024;
    const qint64 mb = 1024 * kb;
    const qint64 gb = 1024 * mb;
    const qint64 tb = 1024 * gb;
    if (bytes >= tb) {
        return QString("%1TB").arg(static_cast<qreal>(bytes) / tb, 0, 'f', 3);
    }
    if (bytes >= gb) {
        return QString("%1GB").arg(static_cast<qreal>(bytes) / gb, 0, 'f', 2);
    }
    if (bytes >= mb) {
        return QString("%1MB").arg(static_cast<qreal>(bytes) / mb, 0, 'f', 1);
    }
    if (bytes >= kb) {
        return QString("%1KB").arg(static_cast<qreal>(bytes) / kb);
    }
    return QString("%1 bytes").arg(bytes);
}

QString
File::absolutePath() const
{
    return p->d.fileInfo.absolutePath();
}

QString
File::dirName() const
{
    return p->d.fileInfo.dir().path();
}

QString
File::baseName() const
{
    return p->d.fileInfo.baseName();
}

QString
File::fileName() const
{
    return p->d.fileInfo.fileName();
}

QString
File::fileName(qint64 frame) const
{
    QString filename = p->d.fileInfo.filePath();
    if (filename.contains(p->d.divider)) {
        QRegularExpression regex(QString("[%1]{1,}").arg(p->d.divider));
        QString formatted = QString::number(frame).rightJustified(filename.count(p->d.divider), '0');
        return filename.replace(regex, formatted);
    }
    else {
        QFileInfo fileInfo(p->d.fileInfo);
        QString path = fileInfo.absolutePath();
        QString basename = fileInfo.completeBaseName();
        QString extension = fileInfo.suffix();
        QString newfilename;
        if (extension.isEmpty()) {
            newfilename = QString("%1.%2").arg(basename).arg(frame);
        }
        else {
            newfilename = QString("%1.%2.%3").arg(basename).arg(frame).arg(extension);
        }
        return QDir(path).filePath(newfilename);
    }
}


QString
File::filePath() const
{
    return p->d.fileInfo.filePath();
}

QString
File::extension() const
{
    return p->d.fileInfo.suffix();
}

qint64
File::size() const
{
    return p->d.fileInfo.size();
}

QString
File::owner() const
{
    return p->d.fileInfo.owner();
}

QString
File::group() const
{
    return p->d.fileInfo.group();
}

QDateTime
File::created() const
{
    return p->d.fileInfo.birthTime();
}

QDateTime
File::modified() const
{
    return p->d.fileInfo.lastModified();
}

bool
File::exists() const
{
    return p->d.fileInfo.exists();
}

bool
File::isReadable() const
{
    return p->d.fileInfo.isReadable();
}

bool
File::isWritable() const
{
    return p->d.fileInfo.isWritable();
}

bool
File::isExecutable() const
{
    return p->d.fileInfo.isExecutable();
}

FileRange
File::fileRange() const
{
    return p->d.fileRange;
}

bool
File::isValid() const
{
    return p->d.fileInfo.exists();
}

void
File::reset()
{
    p.detach();
    p->d.fileInfo = QFileInfo();
}

File::operator QString() const { return filePath(); }

void
File::setFileRange(const FileRange& filerange)
{
    p.detach();
    p->d.fileRange = filerange;
}

File&
File::operator=(const File& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
File::operator==(const File& other) const
{
    return p->d.fileInfo == other.p->d.fileInfo;
}

bool
File::operator!=(const File& other) const
{
    return !(*this == other);
}

QList<File>
File::listDir(const QString& filepath, const QStringList& namefilters, bool ranges)
{
    QList<File> files;
    qint64 frame = 0;
    QString filename;
    FileRange filerange;
    QFileInfoList entries = QDir(filepath).entryInfoList(namefilters, QDir::Files,
                                                         QDir::Type);  // alphabetic to match frames
    for (const QFileInfo& entry : entries) {
        if (ranges) {
            FilePrivate::FilePattern pattern = FilePrivate::pattern(entry.fileName());
            Q_ASSERT("range filename is invalid" && !pattern.filename.isEmpty());
            if (pattern.filename != filename || pattern.frame != frame) {
                if (files.size() && (filerange.size() > 1 || files.back().fileRange().size())) {
                    Q_ASSERT("file range already set" && !files.back().fileRange().size());
                    files.back().setFileRange(FileRange(filerange));
                }
                files.append(File(entry));
                frame = pattern.frame;
                filename = pattern.filename;
                filerange = FileRange();
            }
            filerange.insertFrame(frame, entry);
            if (entry == entries.back() && (filerange.size() > 1 || files.back().fileRange().size())) {
                Q_ASSERT("file range already set" && !files.back().fileRange().size());
                files.back().setFileRange(filerange);
            }
            frame++;
        }
        else {
            files.append(entry);
        }
    }
    return files;
}
}  // namespace flipman::sdk::core
