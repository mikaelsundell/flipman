// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <plugins/quicktimereader.h>
#include <av/smptetime.h>
#include <AVFoundation/AVFoundation.h>
#include <CoreMedia/CoreMedia.h>
#include <mach/mach_time.h>
#include <QFileInfo>
#include <QPointer>

#include <QDebug>

#define DebugCMTime(name, time) \
    qDebug() << name << ": " \
             << "CMTime: PTS Value:" << time.value << ", " \
             << "CMTime: Timescale:" << time.timescale;

#define DebugAVTime(name, time) \
    qDebug() << name << ": " \
             << "AVTime: PTS Value:" << time.ticks() << ", " \
             << "AVTime: Timescale:" << time.timescale();



#define DebugInfo() qDebug() << Q_FUNC_INFO

class QuicktimeReaderPrivate : public QSharedData {
    public:
        QuicktimeReaderPrivate();
        ~QuicktimeReaderPrivate();
        void init();
        bool open(const core::File& file, core::Parameters parameters);
        bool close();
        bool is_open();
        av::Time read();
        av::Time skip();
        av::Time seek(const av::TimeRange& range);
        CMTime to_time(const av::Time& other);
        av::Time to_time(const CMTime& other);
        CMTimeRange to_timerange(const av::TimeRange& other);
        av::TimeRange to_timerange(const CMTimeRange& other);
        static plugins::PluginHandler::Info info();
        static core::Plugin* creator();
        static QList<QString> extensions();
        struct Data
        {
            AVAsset* asset = nil;
            AVAssetReader* reader = nil;
            AVAssetReaderTrackOutput* videooutput = nil;
            core::File file;
            av::TimeRange timerange;
            av::Time startstamp;
            av::Time timestamp;
            av::Fps fps;
            qint32 timescale;
            QString filename;
            QString title;
            core::ImageBuffer image;
            core::AudioBuffer audio;
            core::Parameters metadata;
            core::Error error;
        };
        Data d;
};

QuicktimeReaderPrivate::QuicktimeReaderPrivate()
{
}

QuicktimeReaderPrivate::~QuicktimeReaderPrivate()
{
}

bool
QuicktimeReaderPrivate::open(const core::File& file, core::Parameters parameters)
{
    close();
    NSURL* url = [NSURL fileURLWithPath:file.filepath().toNSString()];
    d.asset = [AVAsset assetWithURL:url];
    if (!d.asset) {
        d.error = core::Error(info().name, QString("unable to load asset from file: %1").arg(file.filepath()));
        qWarning() << "warning: " << d.error.message();
        return false;
    }
    d.file = file;
    NSError* averror = nil;
    d.reader = [[AVAssetReader alloc] initWithAsset:d.asset error:&averror];
    if (!d.reader) {
        d.error = core::Error(info().name, QString("unable to create AVAssetReader for video: %1").arg(QString::fromNSString(averror.localizedDescription)));
        qWarning() << "warning: " << d.error.message();
        return false;
    }
    NSArray<NSString *>* metadataformats = @[
        AVMetadataKeySpaceCommon,
        AVMetadataFormatQuickTimeUserData,
        AVMetadataQuickTimeUserDataKeyAlbum,
        AVMetadataFormatISOUserData,
        AVMetadataISOUserDataKeyCopyright,
        AVMetadataISOUserDataKeyDate,
        AVMetadataFormatQuickTimeMetadata,
        AVMetadataQuickTimeMetadataKeyAuthor,
        AVMetadataFormatiTunesMetadata,
        AVMetadataiTunesMetadataKeyAlbum,
        AVMetadataFormatID3Metadata,
        AVMetadataID3MetadataKeyAudioEncryption,
        AVMetadataKeySpaceIcy,
        AVMetadataIcyMetadataKeyStreamTitle,
        AVMetadataFormatHLSMetadata,
        AVMetadataKeySpaceHLSDateRange,
        AVMetadataKeySpaceAudioFile,
        AVMetadataFormatUnknown
    ];
    for (NSString *format in metadataformats) {
        NSArray<AVMetadataItem *> *metadataForFormat = [d.asset metadataForFormat:format];
        if (metadataForFormat) {
            for (AVMetadataItem* item in metadataForFormat) {
                QString key = QString::fromNSString(item.commonKey);
                QString value = QString::fromNSString(item.value.description);
                if (!key.isEmpty() || !value.isEmpty()) {
                    if (key == "title") { // todo: probably to simple but lets keep it for now
                        d.title = value;
                    }
                    d.metadata.insert(key, value);
                }
            }
        }
    }
    AVAssetTrack* videotrack = [[d.asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!videotrack) {
        d.error = core::Error(info().name, QString("no video track found in file: %1").arg(d.filename));
        qWarning() << "warning: " << d.error.message();
        return false;
    }
    NSArray* formats = [videotrack formatDescriptions];
    for (id formatDesc in formats) {
        CMFormatDescriptionRef desc = (__bridge CMFormatDescriptionRef)formatDesc;
        CMMediaType mediaType = CMFormatDescriptionGetMediaType(desc);
        FourCharCode codecType = CMFormatDescriptionGetMediaSubType(desc);
        NSString* media = [NSString stringWithFormat:@"%c%c%c%c",
                           (mediaType >> 24) & 0xFF,
                           (mediaType >> 16) & 0xFF,
                           (mediaType >> 8) & 0xFF,
                           mediaType & 0xFF];
        
        NSString* codec = [NSString stringWithFormat:@"%c%c%c%c",
                           (codecType >> 24) & 0xFF,
                           (codecType >> 16) & 0xFF,
                           (codecType >> 8) & 0xFF,
                           codecType & 0xFF];
        
        d.metadata.insert("media type", QString::fromNSString(media));
        d.metadata.insert("codec type", QString::fromNSString(codec));
    }
    d.fps = av::Fps::guess(videotrack.nominalFrameRate);
    d.timerange = av::TimeRange::convert(to_timerange(videotrack.timeRange), d.fps);
    d.timestamp = d.timerange.start();
    d.startstamp = d.timestamp;
    AVAssetTrack* timecodetrack = [[d.asset tracksWithMediaType:AVMediaTypeTimecode] firstObject];
    if (timecodetrack) {
        AVAssetReader* timecodereader = [[AVAssetReader alloc] initWithAsset:d.asset error:&averror];
        if (!d.reader) {
            d.error = core::Error(info().name, QString("unable to create AVAssetReader for timecode: %1").arg(QString::fromNSString(averror.localizedDescription)));
            qWarning() << "warning: " << d.error.message();
            return false;
        }
        AVAssetReaderTrackOutput* timecodeoutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:timecodetrack outputSettings:nil];
        [timecodereader addOutput:timecodeoutput];
        bool success = [timecodereader startReading];
        if (success) {
            CMSampleBufferRef samplebuffer = NULL;
            while ((samplebuffer = [timecodeoutput copyNextSampleBuffer])) {
                CMBlockBufferRef blockbuffer = CMSampleBufferGetDataBuffer(samplebuffer);
                CMFormatDescriptionRef formatdescription =  CMSampleBufferGetFormatDescription(samplebuffer);
                if (blockbuffer && formatdescription) {
                    size_t length = 0;
                    size_t totallength = 0;
                    char* data = NULL;
                    OSStatus status = CMBlockBufferGetDataPointer(blockbuffer, 0, &length, &totallength, &data);
                    if (status == kCMBlockBufferNoErr) {
                        CMMediaType type = CMFormatDescriptionGetMediaSubType(formatdescription);
                        uint32_t framequanta = CMTimeCodeFormatDescriptionGetFrameQuanta(formatdescription);
                        uint32_t flags = CMTimeCodeFormatDescriptionGetTimeCodeFlags(formatdescription);
                        bool dropframes = flags & kCMTimeCodeFlag_DropFrame;
                        av::Fps startfps = av::Fps::guess(framequanta);
                        Q_ASSERT("frame quanta does not match" && framequanta == startfps.framequanta());
                        if (dropframes) {
                            if (startfps == av::Fps::fps_24()) {
                                startfps = av::Fps::fps_23_976();
                            }
                            else if (startfps == av::Fps::fps_30()) {
                                startfps = av::Fps::fps_29_97();
                            }
                            else if (startfps == av::Fps::fps_48()) {
                                startfps = av::Fps::fps_47_952();
                            }
                            else if (startfps == av::Fps::fps_60()) {
                                startfps = av::Fps::fps_59_94();
                            }
                        }
                        qint64 frame = 0;
                        Q_ASSERT("drop frames does not match" && dropframes == startfps.drop_frame());
                        if (type == kCMTimeCodeFormatType_TimeCode32) { // 32-bit little-endian to native
                            frame = static_cast<qint64>(EndianS32_BtoN(*reinterpret_cast<int32_t*>(data)));
                        }
                        else if (type == kCMTimeCodeFormatType_TimeCode64) { // 64-bit big-endian to native
                            frame = static_cast<qint64>(EndianS64_BtoN(*reinterpret_cast<int64_t*>(data)));
                        }
                        else {
                            Q_ASSERT("no valid type found for format description" && false);
                        }
                        if (frame) {
                            frame = av::SmpteTime::convert(frame, startfps, d.fps);
                            d.startstamp = av::Time::convert(av::Time(frame, d.fps), d.fps);
                        }
                    }
                    else {
                        d.error = core::Error("quicktimeformat", QString("unable to get data from block buffer for timecode"));
                        qWarning() << "warning: " << d.error.message();
                        return false;
                    }
                }
            }
            if (samplebuffer) {
                CFRelease(samplebuffer);
            }
        }
        else {
            d.error = core::Error(info().name, QString("unable to read sample buffer at for timecode"));
            qWarning() << "warning: " << d.error.message();
            return false;
        }
    }
    d.videooutput = [[AVAssetReaderTrackOutput alloc]
                     initWithTrack:videotrack
                     outputSettings:@{
        (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!d.videooutput) {
        d.error = core::Error(info().name, "unable to create AVAssetReaderTrackOutput");
        qWarning() << "warning: " << d.error.message();
        return false;
    }
    [d.reader addOutput:d.videooutput];
    [d.reader startReading];
    return true;
}

bool
QuicktimeReaderPrivate::close()
{
    if (d.reader) {
        [d.reader cancelReading];
        d.reader = nil;
    }
    d.asset = nil;
    d.videooutput = nil;
    d.timerange.reset();
    d.startstamp.reset();
    d.timestamp.reset();
    d.fps.reset();
    d.timescale = 0;
    d.title = QString();
    d.image.reset();
    d.audio.reset();
    d.metadata.reset();
    d.error.reset();
    return true;
}

bool
QuicktimeReaderPrivate::is_open()
{
    return d.reader != nullptr;
}

av::Time
QuicktimeReaderPrivate::read()
{
    if (!is_open()) {
        d.error = core::Error(info().name, "fauled when trying to read, file must be open");
        qWarning() << "warning: " << d.error.message();
        return av::Time();
    }
    CMSampleBufferRef samplebuffer = nullptr;
    bool iskeyframe = false;
    while (!iskeyframe) {
        if (d.reader.status == AVAssetReaderStatusCompleted) {
            d.error = core::Error(info().name, "end of range, no more samples to copy");
            qWarning() << "warning: " << d.error.message();
            return av::Time();
        }
        samplebuffer = [d.videooutput copyNextSampleBuffer];
        if (!samplebuffer) {
            d.error = core::Error(info().name, "failed when trying to read sample buffer");
            qWarning() << "warning: " << d.error.message();
            return av::Time();
        }
        d.timestamp = av::Time::convert(to_time(CMSampleBufferGetPresentationTimeStamp(samplebuffer)), d.fps);
        iskeyframe = true;
        CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(samplebuffer, true);
        if (attachments) {
            CFDictionaryRef attachment = (CFDictionaryRef)CFArrayGetValueAtIndex(attachments, 0);
            CFBooleanRef notKeyFrame = (CFBooleanRef)CFDictionaryGetValue(attachment, kCMSampleAttachmentKey_NotSync);
            if (notKeyFrame == kCFBooleanTrue) {
                iskeyframe = false;
            }
        }
        if (!iskeyframe) {
            CFRelease(samplebuffer);
        }
    }
    CVImageBufferRef imagebuffer = CMSampleBufferGetImageBuffer(samplebuffer);
    if (!imagebuffer) {
        CFRelease(samplebuffer);
        d.error = core::Error(info().name, "CMSampleBuffer has no image buffer");
        qWarning() << "warning: " << d.error.message();
        return;
    }
    CVPixelBufferLockBaseAddress(imagebuffer, kCVPixelBufferLock_ReadOnly);

    void* baseAddress = CVPixelBufferGetBaseAddress(imagebuffer);
    size_t width = CVPixelBufferGetWidth(imagebuffer);
    size_t height = CVPixelBufferGetHeight(imagebuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imagebuffer);
    size_t totalBytes = bytesPerRow * height;  // Total buffer size

    core::ImageFormat format(core::ImageFormat::UINT8);  // Assuming 8-bit per channel
    QRect datawindow(0, 0, static_cast<int>(width), static_cast<int>(height));
    QRect displaywindow = datawindow;
    int channels = 4; // Assuming ARGB (4 channels)
    d.image = core::ImageBuffer(datawindow, displaywindow, format, channels);
    quint8* bufferData = d.image.data();
    if (bufferData) {
        memcpy(bufferData, baseAddress, totalBytes);
    }
    CVPixelBufferUnlockBaseAddress(imagebuffer, kCVPixelBufferLock_ReadOnly);
    CFRelease(samplebuffer);
    return d.timestamp;
}

av::Time
QuicktimeReaderPrivate::skip()
{
    Q_ASSERT(is_open() || d.reader.status != AVAssetReaderStatusReading);
    
    CMSampleBufferRef samplebuffer = [d.videooutput copyNextSampleBuffer];
    d.timestamp = av::Time::convert(to_time(CMSampleBufferGetPresentationTimeStamp(samplebuffer)), d.fps);
    CFRelease(samplebuffer);
    return d.timestamp;
}

av::Time
QuicktimeReaderPrivate::seek(const av::TimeRange& timerange)
{
    Q_ASSERT(is_open() || d.reader.status != AVAssetReaderStatusReading);
    
    av::Time time = timerange.start();
    Q_ASSERT("ticks are not aligned" && time.ticks() == time.align(time.ticks()));
    if (d.reader) {
        [d.reader cancelReading];
        d.reader = nil;
    }

    NSError* averror = nil;
    d.reader = [[AVAssetReader alloc] initWithAsset:d.asset error:&averror];
    if (!d.reader) {
        d.error = core::Error(info().name, QString("failed to recreate AVAssetReader: %1").arg(QString::fromNSString(averror.localizedDescription)));
        qWarning() << "warning: " << d.error.message();
        return;
    }
    AVAssetTrack* track = [[d.asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
    if (!track) {
        d.error = core::Error(info().name, "no video track found in asset");
        qWarning() << "warning: " << d.error.message();
        return;
    }
    d.videooutput = [[AVAssetReaderTrackOutput alloc]
                     initWithTrack:track
                     outputSettings:@{
        (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!d.videooutput) {
        d.error = core::Error(info().name, "unable to create AVAssetReaderTrackOutput");
        qWarning() << "warning: " << d.error.message();
        return;
    }
    [d.reader addOutput:d.videooutput];
    d.timestamp = time;
    d.reader.timeRange = to_timerange(timerange);
    if (![d.reader startReading]) {
        d.error = core::Error(info().name, "failed to start reading after seeking");
        qWarning() << "warning: " << d.error.message();
        return;
    }
    return d.timestamp;
}

CMTime
QuicktimeReaderPrivate::to_time(const av::Time& other) {
   return CMTimeMakeWithEpoch(other.ticks(), other.timescale(), 0); // default epoch and flags
}

av::Time
QuicktimeReaderPrivate::to_time(const CMTime& other) {
    Q_ASSERT("fps is not valid" && d.fps.is_valid());
    av::Time time;
    if (CMTIME_IS_VALID(other)) {
        time.set_ticks(other.value);
        time.set_timescale(other.timescale);
        time.set_fps(d.fps);
    }
    return time;
}

CMTimeRange
QuicktimeReaderPrivate::to_timerange(const av::TimeRange& other)
{
    CMTime start = to_time(other.start());
    CMTime duration = to_time(other.duration());
    return CMTimeRangeMake(start, duration);
}

av::TimeRange
QuicktimeReaderPrivate::to_timerange(const CMTimeRange& timerange) {
    av::TimeRange range;
    if (CMTIMERANGE_IS_VALID(timerange)) {
        range.set_start(to_time(timerange.start));
        range.set_duration(to_time(timerange.duration));
    }
    return range;
}

plugins::PluginHandler::Info
QuicktimeReaderPrivate::info()
{
    return { "QuicktimeReader", "Reads and decodes QuickTime files using AVFoundation", "1.0.0" };
}

core::Plugin*
QuicktimeReaderPrivate::creator()
{
    return new QuicktimeReader();
}

QList<QString>
QuicktimeReaderPrivate::extensions()
{
    static const QList<QString> extensions = { "mov", "mp4", "m4v", "avi", "mkv", "flv", "mpg", "mpeg", "3gp", "3g2", "mxf", "wmv" };
    return extensions;
}

QuicktimeReader::QuicktimeReader(QObject* parent)
: plugins::MediaReader(parent)
, p(new QuicktimeReaderPrivate())
{
}

QuicktimeReader::~QuicktimeReader()
{
}

bool
QuicktimeReader::open(const core::File& file, core::Parameters parameters)
{
    return p->open(file, parameters);
}

bool
QuicktimeReader::close()
{
    return p->close();
}

bool
QuicktimeReader::is_open() const
{
    return p->is_open();
}

bool
QuicktimeReader::supports_image() const
{
    return true;
}

bool
QuicktimeReader::supports_audio() const
{
    return true;
}

av::Time
QuicktimeReader::read()
{
    return p->read();
}

av::Time
QuicktimeReader::skip()
{
    return p->skip();
}

av::Time
QuicktimeReader::seek(const av::TimeRange& timerange)
{
    return p->seek(timerange);
}

av::Time
QuicktimeReader::start() const
{
    return p->d.startstamp;
}

av::Time
QuicktimeReader::time() const
{
    return p->d.timestamp;
}

av::Fps
QuicktimeReader::fps() const
{
    return p->d.fps;
}

av::TimeRange
QuicktimeReader::timerange() const
{
    return p->d.timerange;
}

QList<QString>
QuicktimeReader::extensions() const
{
    return p->extensions();
}

core::AudioBuffer
QuicktimeReader::audio() const
{
    return core::AudioBuffer();
}

core::ImageBuffer
QuicktimeReader::image() const
{
    return p->d.image;
}

core::Parameters
QuicktimeReader::parameters() const
{
    return core::Parameters();
}

core::Parameters
QuicktimeReader::metadata() const
{
    return core::Parameters();
}

core::Error
QuicktimeReader::error() const
{
    return core::Error();
}

plugins::PluginHandler
QuicktimeReader::handler()
{
    static plugins::PluginHandler handler = plugins::PluginHandler::create<MediaReader>(
        QuicktimeReaderPrivate::info(), QuicktimeReaderPrivate::extensions, QuicktimeReaderPrivate::creator
    );
    return handler;
}
