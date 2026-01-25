// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QDir>
#include <QRegularExpression>
#include <core/file.h>
#include <core/filerange.h>

namespace core {
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
        QFileInfo fileinfo;
        FileRange filerange;
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
        QList<File> files = File::list(path, QStringList() << filter, true);
        if (files.size()) {
            p->d.fileinfo = files.first().p->d.fileinfo;
            p->d.filerange = files.first().p->d.filerange;
        }
        else {
            p->d.fileinfo = QFileInfo(file);
        }
    }
    else {
        p->d.fileinfo = QFileInfo(file);
    }
}

File::File(const QFileInfo& fileinfo)
    : p(new FilePrivate())
{
    p->d.fileinfo = fileinfo;
}

File::~File() {}

QString
File::display_name() const
{
    if (p->d.filerange.size()) {
        FilePrivate::FilePattern pattern = FilePrivate::pattern(filename());
        QString filename = pattern.filename;
        QString range = QString("[%1-%2]").arg(p->d.filerange.start()).arg(p->d.filerange.end());
        return filename.replace(pattern.pos, pattern.length, range);
    }
    else {
        return filename();
    }
}

QString
File::display_size() const
{
    qint64 bytes = p->d.fileinfo.size();
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
File::absolutepath() const
{
    return p->d.fileinfo.absolutePath();
}

QString
File::dirname() const
{
    return p->d.fileinfo.dir().path();
}

QString
File::basename() const
{
    return p->d.fileinfo.baseName();
}

QString
File::filename() const
{
    return p->d.fileinfo.fileName();
}

QString
File::filename(qint64 frame) const
{
    QString filename = p->d.fileinfo.filePath();
    if (filename.contains(p->d.divider)) {
        QRegularExpression regex(QString("[%1]{1,}").arg(p->d.divider));
        QString formatted = QString::number(frame).rightJustified(filename.count(p->d.divider), '0');
        return filename.replace(regex, formatted);
    }
    else {
        QFileInfo fileInfo(p->d.fileinfo);
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
File::filepath() const
{
    return p->d.fileinfo.filePath();
}

QString
File::extension() const
{
    return p->d.fileinfo.suffix();
}

qint64
File::size() const
{
    return p->d.fileinfo.size();
}

QString
File::owner() const
{
    return p->d.fileinfo.owner();
}

QString
File::group() const
{
    return p->d.fileinfo.group();
}

QDateTime
File::created() const
{
    return p->d.fileinfo.birthTime();
}

QDateTime
File::modified() const
{
    return p->d.fileinfo.lastModified();
}

bool
File::exists() const
{
    return p->d.fileinfo.exists();
}

bool
File::is_readable() const
{
    return p->d.fileinfo.isReadable();
}

bool
File::is_writable() const
{
    return p->d.fileinfo.isWritable();
}

bool
File::is_executable() const
{
    return p->d.fileinfo.isExecutable();
}

FileRange
File::filerange() const
{
    return p->d.filerange;
}

void
File::reset()
{
    p.detach();
    p->d.fileinfo = QFileInfo();
}

bool
File::is_valid() const
{
    return p->d.fileinfo.exists();
}

File::operator QString() const { return filepath(); }

void
File::set_filerange(const FileRange& filerange)
{
    p.detach();
    p->d.filerange = filerange;
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
    return p->d.fileinfo == other.p->d.fileinfo;
}

bool
File::operator!=(const File& other) const
{
    return !(*this == other);
}

QList<File>
File::list(const QString& filepath, const QStringList& namefilters, bool ranges)
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
                if (files.size() && (filerange.size() > 1 || files.back().filerange().size())) {
                    Q_ASSERT("file range already set" && !files.back().filerange().size());
                    files.back().set_filerange(FileRange(filerange));
                }
                files.append(File(entry));
                frame = pattern.frame;
                filename = pattern.filename;
                filerange = FileRange();
            }
            filerange.insert_frame(frame, entry);
            if (entry == entries.back() && (filerange.size() > 1 || files.back().filerange().size())) {
                Q_ASSERT("file range already set" && !files.back().filerange().size());
                files.back().set_filerange(filerange);
            }
            frame++;
        }
        else {
            files.append(entry);
        }
    }
    return files;
}
}  // namespace core
