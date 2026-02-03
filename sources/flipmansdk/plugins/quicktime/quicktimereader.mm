// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/plugins/quicktime/quicktime.h>

#include <flipmansdk/av/smptetime.h>

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

namespace flipman::sdk::plugins {

class QuicktimeReaderPrivate : public QSharedData {
public:
    QuicktimeReaderPrivate();
    ~QuicktimeReaderPrivate();
    void init();
    void loadAsset();
    bool open(const core::File& file, core::Parameters parameters);
    bool close();
    bool isOpen();
    av::Time read();
    av::Time skip();
    av::Time seek(const av::TimeRange& timeRange);
    CMTime toTime(const av::Time& other);
    av::Time toTime(const CMTime& other);
    CMTimeRange toTimerange(const av::TimeRange& other);
    av::TimeRange toTimeRange(const CMTimeRange& other);
    static plugins::PluginHandler::Info info();
    static core::Plugin* creator();
    static QList<QString> extensions();
    struct Data
    {
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* videoOutput = nil;
        AVAssetTrack* videoTrack = nil;
        AVAssetTrack* timeCodeTrack = nil;
        core::File file;
        av::TimeRange timeRange;
        av::Time startStamp;
        av::Time timeStamp;
        av::Fps fps;
        qint32 timeScale;
        QString fileName;
        QString title;
        core::ImageBuffer image;
        core::AudioBuffer audio;
        core::Parameters metaData;
        core::Error error;
        QPointer<QuicktimeReader> object;
    };
    Data d;
};

QuicktimeReaderPrivate::QuicktimeReaderPrivate()
{
}

QuicktimeReaderPrivate::~QuicktimeReaderPrivate()
{
}

void
QuicktimeReaderPrivate::init()
{
}

void QuicktimeReaderPrivate::loadAsset()
{
    dispatch_group_t group = dispatch_group_create();
    
    __block AVAssetTrack* videoTrack = nil;
    __block AVAssetTrack* timeCodeTrack = nil;
    
    dispatch_group_enter(group);
    [d.asset loadTracksWithMediaType:AVMediaTypeVideo
                   completionHandler:^(NSArray<AVAssetTrack*>* tracks, NSError*) {
        if (tracks.count)
            videoTrack = tracks.firstObject;
        dispatch_group_leave(group);
    }];
    
    dispatch_group_enter(group);
    [d.asset loadTracksWithMediaType:AVMediaTypeTimecode
                   completionHandler:^(NSArray<AVAssetTrack*>* tracks, NSError*) {
        if (tracks.count)
            timeCodeTrack = tracks.firstObject;
        dispatch_group_leave(group);
    }];
    
    for (NSString* format in d.asset.availableMetadataFormats) {
        dispatch_group_enter(group);
        [d.asset loadMetadataForFormat:format
                     completionHandler:^(NSArray<AVMetadataItem*>* items,
                                         NSError* error) {
            
            if (!error) {
                for (AVMetadataItem* item in items) {
                    if (!item.commonKey || !item.value)
                        continue;
                    d.metaData.insert(
                                      QString::fromNSString(item.commonKey),
                                      QString::fromNSString(item.value.description));
                }
            }
            dispatch_group_leave(group);
        }];
    }
    dispatch_group_notify(
        group,
        dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
        ^{

        NSError* error = nil;
        d.reader = [[AVAssetReader alloc] initWithAsset:d.asset error:&error];
        if (!d.reader) {
            d.error = core::Error(info().name,
                                  QString::fromNSString(error.localizedDescription));
            return;
        }
        NSArray* formats = [videoTrack formatDescriptions];
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
            
            d.metaData.insert("media type", QString::fromNSString(media));
            d.metaData.insert("codec type", QString::fromNSString(codec));
        }
        if (!videoTrack) {
            d.error = core::Error(info().name, "no video track");
            return;
        }
        d.videoTrack = videoTrack;
        d.fps = av::Fps::guess(videoTrack.nominalFrameRate);
        d.timeRange = av::TimeRange::convert(
                                             toTimeRange(videoTrack.timeRange), d.fps);
        d.startStamp = d.timeRange.start();
        d.timeStamp  = d.startStamp;
        if (timeCodeTrack) {
            AVAssetReader* timeCodeReader = [[AVAssetReader alloc] initWithAsset:d.asset error:&error];
            if (!d.reader) {
                d.error = core::Error(info().name, QString("unable to create AVAssetReader for timecode: %1").arg(QString::fromNSString(error.localizedDescription)));
                return;
            }
            AVAssetReaderTrackOutput* timeCodeOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:timeCodeTrack outputSettings:nil];
            [timeCodeReader addOutput:timeCodeOutput];
            bool success = [timeCodeReader startReading];
            if (success) {
                CMSampleBufferRef sampleBuffer = NULL;
                while ((sampleBuffer = [timeCodeOutput copyNextSampleBuffer])) {
                    CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
                    CMFormatDescriptionRef formatDescription =  CMSampleBufferGetFormatDescription(sampleBuffer);
                    if (blockBuffer && formatDescription) {
                        size_t length = 0;
                        size_t totalLength = 0;
                        char* data = NULL;
                        OSStatus status = CMBlockBufferGetDataPointer(blockBuffer, 0, &length, &totalLength, &data);
                        if (status == kCMBlockBufferNoErr) {
                            CMMediaType type = CMFormatDescriptionGetMediaSubType(formatDescription);
                            uint32_t frameQuanta = CMTimeCodeFormatDescriptionGetFrameQuanta(formatDescription);
                            uint32_t flags = CMTimeCodeFormatDescriptionGetTimeCodeFlags(formatDescription);
                            bool dropFrames = flags & kCMTimeCodeFlag_DropFrame;
                            av::Fps startFps = av::Fps::guess(frameQuanta);
                            Q_ASSERT("frame quanta does not match" && frameQuanta == startFps.frameQuanta());
                            if (dropFrames) {
                                if (startFps == av::Fps::fps24()) {
                                    startFps = av::Fps::fps23_976();
                                }
                                else if (startFps == av::Fps::fps30()) {
                                    startFps = av::Fps::fps29_97();
                                }
                                else if (startFps == av::Fps::fps48()) {
                                    startFps = av::Fps::fps47_952();
                                }
                                else if (startFps == av::Fps::fps60()) {
                                    startFps = av::Fps::fps59_94();
                                }
                            }
                            qint64 frame = 0;
                            Q_ASSERT("drop frames does not match" && dropFrames == startFps.dropFrame());
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
                                frame = av::SmpteTime::convert(frame, startFps, d.fps);
                                d.startStamp = av::Time::convert(av::Time(frame, d.fps), d.fps);
                            }
                        }
                        else {
                            d.error = core::Error("quicktimeformat", QString("unable to get data from block buffer for timecode"));
                            return;
                        }
                    }
                }
                if (sampleBuffer) {
                    CFRelease(sampleBuffer);
                }
            }
            else {
                d.error = core::Error(info().name, QString("unable to read sample buffer at for timecode"));
                return;
            }
            d.timeCodeTrack = timeCodeTrack;
        }
        d.videoOutput =
        [[AVAssetReaderTrackOutput alloc]
         initWithTrack:videoTrack
         outputSettings:@{
            (NSString*)kCVPixelBufferPixelFormatTypeKey :
                @(kCVPixelFormatType_32BGRA)
        }];
        
        [d.reader addOutput:d.videoOutput];
        if (![d.reader startReading]) {
            d.error = core::Error(info().name, "reader failed to start");
            return;
        }
        QMetaObject::invokeMethod(
            d.object,
            &QuicktimeReader::opened,
            Qt::QueuedConnection
        );
    });
}

bool
QuicktimeReaderPrivate::open(const core::File& file, core::Parameters)
{
    close();
    d.file = file;
    d.error.reset();
    
    NSURL* url = [NSURL fileURLWithPath:file.filePath().toNSString()];
    d.asset = [AVURLAsset URLAssetWithURL:url options:nil];
    
    if (!d.asset) {
        d.error = core::Error(info().name, "failed to create AVAsset");
        return false;
    }
    NSArray* keys = @[
        @"tracks",
        @"duration",
        @"commonMetadata",
        @"availableMetadataFormats"
    ];
    [d.asset loadValuesAsynchronouslyForKeys:keys completionHandler:^{
        NSError* error = nil;
        for (NSString* key in keys) {
            if ([d.asset statusOfValueForKey:key error:&error]
                != AVKeyValueStatusLoaded) {

                d.error = core::Error(
                    info().name,
                    QString::fromNSString(error.localizedDescription));
                return;
            }
        }
        this->loadAsset();
    }];
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
    d.videoOutput = nil;
    d.timeRange.reset();
    d.startStamp.reset();
    d.timeStamp.reset();
    d.fps.reset();
    d.timeScale = 0;
    d.title = QString();
    d.image.reset();
    d.audio.reset();
    d.metaData.reset();
    d.error.reset();
    return true;
}

bool
QuicktimeReaderPrivate::isOpen()
{
    return d.reader != nullptr && d.videoOutput != nil;
}

av::Time
QuicktimeReaderPrivate::read()
{
    if (!isOpen()) {
        d.error = core::Error(info().name, "failed when trying to read, file must be open");
        qWarning() << "warning: " << d.error.message();
        return av::Time();
    }
    CMSampleBufferRef sampleBuffer = nullptr;
    bool isKeyFrame = false;
    while (!isKeyFrame) {
        if (d.reader.status == AVAssetReaderStatusCompleted) {
            d.error = core::Error(info().name, "end of range, no more samples to copy");
            qWarning() << "warning: " << d.error.message();
            return av::Time();
        }
        sampleBuffer = [d.videoOutput copyNextSampleBuffer];
        if (!sampleBuffer) {
            d.error = core::Error(info().name, "failed when trying to read sample buffer");
            qWarning() << "warning: " << d.error.message();
            return av::Time();
        }
        d.timeStamp = av::Time::convert(toTime(CMSampleBufferGetPresentationTimeStamp(sampleBuffer)), d.fps);
        isKeyFrame = true;
        CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true);
        if (attachments) {
            CFDictionaryRef attachment = (CFDictionaryRef)CFArrayGetValueAtIndex(attachments, 0);
            CFBooleanRef notKeyFrame = (CFBooleanRef)CFDictionaryGetValue(attachment, kCMSampleAttachmentKey_NotSync);
            if (notKeyFrame == kCFBooleanTrue) {
                isKeyFrame = false;
            }
        }
        if (!isKeyFrame) {
            CFRelease(sampleBuffer);
        }
    }
    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!imageBuffer) {
        CFRelease(sampleBuffer);
        d.error = core::Error(info().name, "CMSampleBuffer has no image buffer");
        qWarning() << "warning: " << d.error.message();
        return av::Time();
    }
    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    
    void* baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
    size_t width = CVPixelBufferGetWidth(imageBuffer);
    size_t height = CVPixelBufferGetHeight(imageBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
    size_t totalBytes = bytesPerRow * height;
    
    core::ImageFormat format(core::ImageFormat::UINT8);  // Assuming 8-bit per channel
    QRect dataWindow(0, 0, static_cast<int>(width), static_cast<int>(height));
    QRect displayWindow = dataWindow;
    int channels = 4; // Assuming ARGB (4 channels)
    d.image = core::ImageBuffer(dataWindow, displayWindow, format, channels);
    quint8* bufferData = d.image.data();
    if (bufferData) {
        memcpy(bufferData, baseAddress, totalBytes);
    }
    CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
    CFRelease(sampleBuffer);
    return d.timeStamp;
}

av::Time
QuicktimeReaderPrivate::skip()
{
    Q_ASSERT(isOpen() || d.reader.status != AVAssetReaderStatusReading);
    
    CMSampleBufferRef sampleBuffer = [d.videoOutput copyNextSampleBuffer];
    d.timeStamp = av::Time::convert(toTime(CMSampleBufferGetPresentationTimeStamp(sampleBuffer)), d.fps);
    CFRelease(sampleBuffer);
    return d.timeStamp;
}

av::Time
QuicktimeReaderPrivate::seek(const av::TimeRange& timerange)
{
    Q_ASSERT(isOpen() || d.reader.status != AVAssetReaderStatusReading);
    
    av::Time time = timerange.start();
    Q_ASSERT("ticks are not aligned" && time.ticks() == time.align(time.ticks()));
    if (d.reader) {
        [d.reader cancelReading];
        d.reader = nil;
    }
    
    NSError* error = nil;
    d.reader = [[AVAssetReader alloc] initWithAsset:d.asset error:&error];
    if (!d.reader) {
        d.error = core::Error(info().name, QString("failed to recreate AVAssetReader: %1").arg(QString::fromNSString(error.localizedDescription)));
        qWarning() << "warning: " << d.error.message();
        return;
    }
    if (!d.videoTrack) {
        d.error = core::Error(info().name, "no video track found in asset");
        qWarning() << "warning: " << d.error.message();
        return;
    }
    d.videoOutput = [[AVAssetReaderTrackOutput alloc]
                     initWithTrack:d.videoTrack
                     outputSettings:@{
        (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
    }];
    if (!d.videoOutput) {
        d.error = core::Error(info().name, "unable to create AVAssetReaderTrackOutput");
        qWarning() << "warning: " << d.error.message();
        return;
    }
    [d.reader addOutput:d.videoOutput];
    d.timeStamp = time;
    d.reader.timeRange = toTimerange(timerange);
    if (![d.reader startReading]) {
        d.error = core::Error(info().name, "failed to start reading after seeking");
        qWarning() << "warning: " << d.error.message();
        return;
    }
    return d.timeStamp;
}

CMTime
QuicktimeReaderPrivate::toTime(const av::Time& other) {
    return CMTimeMakeWithEpoch(other.ticks(), other.timeScale(), 0); // default epoch and flags
}

av::Time
QuicktimeReaderPrivate::toTime(const CMTime& other) {
    Q_ASSERT("fps is not valid" && d.fps.isValid());
    av::Time time;
    if (CMTIME_IS_VALID(other)) {
        time.setTicks(other.value);
        time.setTimeScale(other.timescale);
        time.setFps(d.fps);
    }
    return time;
}

CMTimeRange
QuicktimeReaderPrivate::toTimerange(const av::TimeRange& other)
{
    CMTime start = toTime(other.start());
    CMTime duration = toTime(other.duration());
    return CMTimeRangeMake(start, duration);
}

av::TimeRange
QuicktimeReaderPrivate::toTimeRange(const CMTimeRange& timeRange) {
    av::TimeRange range;
    if (CMTIMERANGE_IS_VALID(timeRange)) {
        range.setStart(toTime(timeRange.start));
        range.setDuration(toTime(timeRange.duration));
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
    p->d.object = this;
    p->init();
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
QuicktimeReader::isOpen() const
{
    return p->isOpen();
}

bool
QuicktimeReader::supportsImage() const
{
    return true;
}

bool
QuicktimeReader::supportsAudio() const
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
    return p->d.startStamp;
}

av::Time
QuicktimeReader::time() const
{
    return p->d.timeStamp;
}

av::Fps
QuicktimeReader::fps() const
{
    return p->d.fps;
}

av::TimeRange
QuicktimeReader::timeRange() const
{
    return p->d.timeRange;
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
QuicktimeReader::metaData() const
{
    return p->d.metaData;
}

core::Error
QuicktimeReader::error() const
{
    return p->d.error;
}

plugins::PluginHandler
QuicktimeReader::handler()
{
    static plugins::PluginHandler handler = plugins::PluginHandler::create<MediaReader>(
                                                                                        QuicktimeReaderPrivate::info(), QuicktimeReaderPrivate::extensions, QuicktimeReaderPrivate::creator
                                                                                        );
    return handler;
}

}
