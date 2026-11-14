// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/core/log.h>
#include <flipmansdk/core/perfscope.h>
#include <flipmansdk/core/profile.h>
#include <flipmansdk/plugins/quicktime/quicktimereader.h>
#include <AVFoundation/AVFoundation.h>
#include <CoreMedia/CoreMedia.h>
#include <mach/mach_time.h>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>
#include <QScopeGuard>
#include <algorithm>
#include <atomic>

#define DebugCMTime(name, time)                                                         \
    qCDebug(flipman::sdk::core::mediaLog) << name << ": "                               \
                                          << "CMTime: PTS Value:" << time.value << ", " \
                                          << "CMTime: Timescale:" << time.timescale;

#define DebugAVTime(name, time)                                                           \
    qCDebug(flipman::sdk::core::mediaLog) << name << ": "                                 \
                                          << "AVTime: PTS Value:" << time.ticks() << ", " \
                                          << "AVTime: Timescale:" << time.timescale();

#define DebugInfo() qCDebug(flipman::sdk::core::mediaLog) << Q_FUNC_INFO

namespace flipman::sdk::plugins {

class QuicktimeReaderPrivate : public QSharedData {
public:
    QuicktimeReaderPrivate();
    ~QuicktimeReaderPrivate();

    void init();
    void loadAsset(quint64 generation);

    bool open(const core::File& file, const MediaReader::Options& options);
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

    struct Data {
        dispatch_group_t loading = dispatch_group_create();
        AVAsset* asset = nil;
        AVAssetReader* reader = nil;
        AVAssetReaderTrackOutput* videoOutput = nil;
        AVAssetTrack* videoTrack = nil;
        AVAssetTrack* timeCodeTrack = nil;
        core::File file;
        core::ImageBuffer image;
        core::AudioBuffer audio;
        core::MetaData metaData;
        core::Error error;
        av::TimeRange timeRange;
        av::Time startStamp;
        av::Time timeStamp;
        av::Fps fps;
        QString fileName;
        QString title;
        std::atomic<quint64> generation = 0;
        std::atomic<bool> open = false;
        qint32 timeScale = 0;
        QPointer<QuicktimeReader> object;
    };
    Data d;
};

QuicktimeReaderPrivate::QuicktimeReaderPrivate() {}

QuicktimeReaderPrivate::~QuicktimeReaderPrivate() {}

void
QuicktimeReaderPrivate::init()
{}

static QString
pixelFormatName(OSType format)
{
    switch (format) {
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange: return "NV12 VideoRange";
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange: return "NV12 FullRange";
    case kCVPixelFormatType_32BGRA: return "BGRA";
    case kCVPixelFormatType_422YpCbCr8: return "UYVY 422";
    default: return QString("OSType(0x%1)").arg(quint32(format), 8, 16, QLatin1Char('0'));
    }
}

static NSDictionary*
videoOutputSettings()
{
    return @{(NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};
}

static QString
cfString(CFTypeRef value)
{
    if (!value)
        return {};

    if (CFGetTypeID(value) == CFStringGetTypeID())
        return QString::fromCFString(static_cast<CFStringRef>(value));

    CFStringRef desc = CFCopyDescription(value);
    const QString result = desc ? QString::fromCFString(desc) : QString();

    if (desc)
        CFRelease(desc);

    return result;
}

static QString
attachmentString(CVPixelBufferRef pixelBuffer, CFStringRef key)
{
    CFTypeRef value = CVBufferCopyAttachment(pixelBuffer, key, nullptr);
    const QString result = cfString(value);
    if (value)
        CFRelease(value);
    return result;
}

static void
debugPixelBufferColorInfo(CVPixelBufferRef pixelBuffer)
{
    if (!pixelBuffer || !core::mediaLog().isDebugEnabled())
        return;

    qCDebug(flipman::sdk::core::mediaLog) << "quicktimereader: pixel buffer";
    qCDebug(flipman::sdk::core::mediaLog)
        << "  format:" << pixelFormatName(CVPixelBufferGetPixelFormatType(pixelBuffer));
    qCDebug(flipman::sdk::core::mediaLog)
        << "  size:" << CVPixelBufferGetWidth(pixelBuffer) << "x" << CVPixelBufferGetHeight(pixelBuffer);
    qCDebug(flipman::sdk::core::mediaLog) << "  planes:" << CVPixelBufferGetPlaneCount(pixelBuffer);

    qCDebug(flipman::sdk::core::mediaLog)
        << "  primaries:" << attachmentString(pixelBuffer, kCVImageBufferColorPrimariesKey);

    qCDebug(flipman::sdk::core::mediaLog)
        << "  transfer:" << attachmentString(pixelBuffer, kCVImageBufferTransferFunctionKey);

    qCDebug(flipman::sdk::core::mediaLog)
        << "  ycbcr matrix:" << attachmentString(pixelBuffer, kCVImageBufferYCbCrMatrixKey);

    qCDebug(flipman::sdk::core::mediaLog)
        << "  gamma level:" << attachmentString(pixelBuffer, kCVImageBufferGammaLevelKey);
}

static void
debugImageColorInfo(const core::ImageBuffer& image)
{
    if (!core::mediaLog().isDebugEnabled())
        return;

    qCDebug(flipman::sdk::core::mediaLog)
        << "quicktimereader: tagged image"
        << "colorSpace" << int(image.colorSpace()) << "transfer" << int(image.transferFunction()) << "pixelRange"
        << int(image.pixelRange()) << "pixelLayout" << int(image.pixelLayout()) << "packing" << int(image.packing())
        << "subsampling" << int(image.subsampling());
}

static render::ColorSpace
colorSpaceFromPixelBuffer(CVPixelBufferRef pixelBuffer)
{
    const QString primaries = attachmentString(pixelBuffer, kCVImageBufferColorPrimariesKey);

    if (primaries.contains("2020", Qt::CaseInsensitive))
        return render::ColorSpace::Rec2020;

    if (primaries.contains("P3", Qt::CaseInsensitive))
        return render::ColorSpace::DisplayP3;

    if (primaries.contains("709", Qt::CaseInsensitive))
        return render::ColorSpace::Rec709;

    return render::ColorSpace::Unknown;
}

static render::TransferFunction
transferFunctionFromPixelBuffer(CVPixelBufferRef pixelBuffer)
{
    const QString transfer = attachmentString(pixelBuffer, kCVImageBufferTransferFunctionKey);

    if (transfer.contains("Apple", Qt::CaseInsensitive) && transfer.contains("Log", Qt::CaseInsensitive))
        return render::TransferFunction::AppleLog;

    if (transfer.contains("sRGB", Qt::CaseInsensitive))
        return render::TransferFunction::SRGB;

    if (transfer.contains("Linear", Qt::CaseInsensitive))
        return render::TransferFunction::Linear;

    if (transfer.contains("709", Qt::CaseInsensitive))
        return render::TransferFunction::Gamma24;

    return render::TransferFunction::Unknown;
}

static void
tagImageColorInfo(CVPixelBufferRef pixelBuffer, core::ImageBuffer& image)
{
    image.setColorSpace(colorSpaceFromPixelBuffer(pixelBuffer));
    image.setTransferFunction(transferFunctionFromPixelBuffer(pixelBuffer));
}

static bool
copyPixelBufferToImageBuffer(CVPixelBufferRef pixelBuffer, core::ImageBuffer& image, core::Error& error)
{
    FM_PROFILE_SCOPE("quicktime", "copy_pixel_buffer");
    if (!pixelBuffer) {
        error = core::Error("quicktimereader", "invalid CVPixelBuffer");
        return false;
    }

    const OSType pixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer);

    static std::atomic<bool> printedPixelBufferInfo { false };

    if (core::mediaLog().isDebugEnabled() && !printedPixelBufferInfo.exchange(true, std::memory_order_relaxed)) {
        debugPixelBufferColorInfo(pixelBuffer);
    }

    const int width = int(CVPixelBufferGetWidth(pixelBuffer));
    const int height = int(CVPixelBufferGetHeight(pixelBuffer));
    const QRect dataWindow(0, 0, width, height);
    const QRect displayWindow = dataWindow;

    if (pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
        || pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) {
        image = core::ImageBuffer(dataWindow, displayWindow, core::ImageFormat(core::ImageFormat::UInt8), 1);

        image.setPacking(core::ImageBuffer::Packing::BiPlanar);
        image.setSubsampling(core::ImageBuffer::Subsampling::CS420);
        image.setPixelLayout(core::ImageBuffer::PixelLayout::NV12);
        image.setPixelRange(pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
                                ? core::ImageBuffer::PixelRange::Full
                                : core::ImageBuffer::PixelRange::Video);
        tagImageColorInfo(pixelBuffer, image);
        debugImageColorInfo(image);
        image.allocate();

        quint8* dstY = image.planeData(0);
        quint8* dstUV = image.planeData(1);

        const quint8* srcY = static_cast<const quint8*>(CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0));
        const quint8* srcUV = static_cast<const quint8*>(CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1));

        const size_t srcStrideY = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
        const size_t srcStrideUV = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1);

        const size_t dstStrideY = image.planeStride(0);
        const size_t dstStrideUV = image.planeStride(1);

        const QSize uvSize = image.planeSize(1);

        for (int y = 0; y < height; ++y) {
            const size_t bytes = std::min(dstStrideY, srcStrideY);
            memcpy(dstY + size_t(y) * dstStrideY, srcY + size_t(y) * srcStrideY, bytes);
        }

        for (int y = 0; y < uvSize.height(); ++y) {
            const size_t bytes = std::min(dstStrideUV, srcStrideUV);
            memcpy(dstUV + size_t(y) * dstStrideUV, srcUV + size_t(y) * srcStrideUV, bytes);
        }

        return true;
    }

    if (pixelFormat == kCVPixelFormatType_32BGRA) {
        image = core::ImageBuffer(dataWindow, displayWindow, core::ImageFormat(core::ImageFormat::UInt8), 4);

        image.setPacking(core::ImageBuffer::Packing::Interleaved);
        image.setSubsampling(core::ImageBuffer::Subsampling::None);
        image.setPixelLayout(core::ImageBuffer::PixelLayout::BGRA);
        image.setPixelRange(core::ImageBuffer::PixelRange::Full);
        tagImageColorInfo(pixelBuffer, image);
        debugImageColorInfo(image);
        image.allocate();

        const quint8* src = static_cast<const quint8*>(CVPixelBufferGetBaseAddress(pixelBuffer));
        const size_t srcStride = CVPixelBufferGetBytesPerRow(pixelBuffer);
        const size_t dstStride = image.strideSize();

        quint8* dst = image.data();

        for (int y = 0; y < height; ++y) {
            memcpy(dst + size_t(y) * dstStride, src + size_t(y) * srcStride, std::min(dstStride, srcStride));
        }

        return true;
    }

    if (pixelFormat == kCVPixelFormatType_422YpCbCr8) {
        image = core::ImageBuffer(dataWindow, displayWindow, core::ImageFormat(core::ImageFormat::UInt8), 2);

        image.setPacking(core::ImageBuffer::Packing::Packed);
        image.setSubsampling(core::ImageBuffer::Subsampling::CS422);
        image.setPixelLayout(core::ImageBuffer::PixelLayout::UYVY);
        image.setPixelRange(core::ImageBuffer::PixelRange::Video);
        tagImageColorInfo(pixelBuffer, image);
        debugImageColorInfo(image);
        image.allocate();

        const quint8* src = static_cast<const quint8*>(CVPixelBufferGetBaseAddress(pixelBuffer));
        const size_t srcStride = CVPixelBufferGetBytesPerRow(pixelBuffer);
        const size_t dstStride = image.strideSize();

        quint8* dst = image.data();

        for (int y = 0; y < height; ++y) {
            memcpy(dst + size_t(y) * dstStride, src + size_t(y) * srcStride, std::min(dstStride, srcStride));
        }

        return true;
    }

    error = core::Error("quicktimereader",
                        QString("unsupported CVPixelBuffer format: %1").arg(pixelFormatName(pixelFormat)));
    return false;
}

void
QuicktimeReaderPrivate::loadAsset(quint64 generation)
{
    FM_PROFILE_SCOPE("quicktime", "load_asset");
    core::PerfScope phase("quicktime.load_asset");
    dispatch_group_t group = dispatch_group_create();

    NSMutableArray<AVMetadataItem*>* metadata = [NSMutableArray array];
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
                     completionHandler:^(NSArray<AVMetadataItem*>* items, NSError* error) {
                         if (!error && items) {
                             @synchronized(metadata) {
                                 [metadata addObjectsFromArray:items];
                             }
                         }

                         dispatch_group_leave(group);
                     }];
    }

    dispatch_group_notify(group, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        const auto finished = qScopeGuard([this]() { dispatch_group_leave(d.loading); });

        // Publish metadata once, after all format callbacks have completed.
        for (AVMetadataItem* item in metadata) {
            if (item.commonKey && item.value)
                d.metaData.insert(QString::fromNSString(item.commonKey), QString::fromNSString(item.value.description));
        }
        NSError* error = nil;

        d.reader = [[AVAssetReader alloc] initWithAsset:d.asset error:&error];

        if (!d.reader) {
            d.error = core::Error(info().name, QString::fromNSString(error.localizedDescription));
            return;
        }

        NSArray* formats = [videoTrack formatDescriptions];

        for (id formatDesc in formats) {
            CMFormatDescriptionRef desc = (__bridge CMFormatDescriptionRef)formatDesc;
            CMMediaType mediaType = CMFormatDescriptionGetMediaType(desc);
            FourCharCode codecType = CMFormatDescriptionGetMediaSubType(desc);

            NSString* media = [NSString stringWithFormat:@"%c%c%c%c", (mediaType >> 24) & 0xFF,
                                                         (mediaType >> 16) & 0xFF, (mediaType >> 8) & 0xFF,
                                                         mediaType & 0xFF];

            NSString* codec = [NSString stringWithFormat:@"%c%c%c%c", (codecType >> 24) & 0xFF,
                                                         (codecType >> 16) & 0xFF, (codecType >> 8) & 0xFF,
                                                         codecType & 0xFF];

            d.metaData.insert("media type", QString::fromNSString(media));
            d.metaData.insert("codec type", QString::fromNSString(codec));
        }

        if (!videoTrack) {
            d.error = core::Error(info().name, "no video track");
            return;
        }

        d.videoTrack = videoTrack;
        const auto nominalFps = av::Fps::guess(videoTrack.nominalFrameRate);

        // A fractional frame rate does not imply drop-frame timecode. Use the
        // timecode track's explicit flag below; untagged clips default to NDF.
        d.fps = av::Fps(qint32(nominalFps.numerator()), nominalFps.denominator(), false);
        d.timeRange = av::TimeRange::convert(toTimeRange(videoTrack.timeRange), d.fps);
        d.startStamp = d.timeRange.start();
        d.timeStamp = d.startStamp;

        if (timeCodeTrack) {
            AVAssetReader* timeCodeReader = [[AVAssetReader alloc] initWithAsset:d.asset error:&error];

            if (!timeCodeReader) {
                d.error = core::Error(info().name, QString("unable to create AVAssetReader for timecode: %1")
                                                       .arg(QString::fromNSString(error.localizedDescription)));
                return;
            }

            AVAssetReaderTrackOutput* timeCodeOutput =
                [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:timeCodeTrack outputSettings:nil];

            [timeCodeReader addOutput:timeCodeOutput];

            const bool success = [timeCodeReader startReading];

            if (success) {
                CMSampleBufferRef sampleBuffer = NULL;

                while ((sampleBuffer = [timeCodeOutput copyNextSampleBuffer])) {
                    CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
                    CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(sampleBuffer);

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
                            d.fps = av::Fps(qint32(d.fps.numerator()), d.fps.denominator(), dropFrames);
                            d.timeRange = av::TimeRange::convert(toTimeRange(videoTrack.timeRange), d.fps);
                            d.startStamp = d.timeRange.start();

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

                            if (type == kCMTimeCodeFormatType_TimeCode32) {
                                frame = static_cast<qint64>(EndianS32_BtoN(*reinterpret_cast<int32_t*>(data)));
                            }
                            else if (type == kCMTimeCodeFormatType_TimeCode64) {
                                frame = static_cast<qint64>(EndianS64_BtoN(*reinterpret_cast<int64_t*>(data)));
                            }
                            else {
                                Q_ASSERT("no valid type found for format description" && false);
                            }

                            if (frame) {
                                frame = av::SmpteTime::convert(frame, startFps, d.fps);
                                d.startStamp = av::Time::convert(av::Time::fromFrames(frame, d.fps), d.fps);
                            }
                        }
                        else {
                            d.error = core::Error("quicktimeformat",
                                                  "unable to get data from block buffer for timecode");
                            return;
                        }
                    }

                    CFRelease(sampleBuffer);
                }
            }
            else {
                d.error = core::Error(info().name, "unable to read sample buffer at for timecode");
                return;
            }

            d.timeCodeTrack = timeCodeTrack;
        }

        d.videoOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack
                                                         outputSettings:videoOutputSettings()];

        [d.reader addOutput:d.videoOutput];

        if (![d.reader startReading]) {
            d.error = core::Error(info().name, "reader failed to start");
            return;
        }

        d.open = true;
        QMetaObject::invokeMethod(
            d.object,
            [this, generation]() {
                if (d.generation == generation && d.open && d.object)
                    Q_EMIT d.object->opened();
            },
            Qt::QueuedConnection);
    });
}

bool
QuicktimeReaderPrivate::open(const core::File& file, const MediaReader::Options& options)
{
    Q_UNUSED(options);

    close();

    d.file = file;
    d.error.reset();

    NSURL* url = [NSURL fileURLWithPath:file.filePath().toNSString()];
    d.asset = [AVURLAsset URLAssetWithURL:url options:nil];

    if (!d.asset) {
        d.error = core::Error(info().name, "failed to create AVAsset");
        return false;
    }

    NSArray* keys = @[ @"tracks", @"duration", @"commonMetadata", @"availableMetadataFormats" ];

    const auto generation = d.generation.load();
    dispatch_group_enter(d.loading);
    [d.asset
        loadValuesAsynchronouslyForKeys:keys
                      completionHandler:^{
                          NSError* error = nil;

                          for (NSString* key in keys) {
                              if ([d.asset statusOfValueForKey:key error:&error] != AVKeyValueStatusLoaded) {
                                  d.error = core::Error(info().name, QString::fromNSString(error.localizedDescription));
                                  dispatch_group_leave(d.loading);
                                  return;
                              }
                          }

                          this->loadAsset(generation);
                      }];

    return true;
}

bool
QuicktimeReaderPrivate::close()
{
    ++d.generation;

    // AVFoundation callbacks borrow this private object. Cancel outstanding
    // loading and drain callbacks before resetting or destroying their state.
    // Normal frame reads do not wait on this group.
    [d.asset cancelLoading];
    dispatch_group_wait(d.loading, DISPATCH_TIME_FOREVER);
    d.open = false;
    if (d.reader) {
        [d.reader cancelReading];
        d.reader = nil;
    }

    d.asset = nil;
    d.videoOutput = nil;
    d.videoTrack = nil;
    d.timeCodeTrack = nil;
    d.timeRange.reset();
    d.startStamp.reset();
    d.timeStamp.reset();
    d.fps.reset();
    d.timeScale = 0;
    d.title.clear();
    d.image.reset();
    d.audio.reset();
    d.metaData.reset();
    d.error.reset();

    return true;
}

bool
QuicktimeReaderPrivate::isOpen()
{
    return d.open;
}

av::Time
QuicktimeReaderPrivate::read()
{
    FM_PROFILE_SCOPE("quicktime", "read");
    core::PerfScope phase("quicktime.read");
    if (!isOpen()) {
        d.error = core::Error(info().name, "failed when trying to read, file must be open");
        qWarning() << "warning:" << d.error.message();
        return av::Time();
    }

    CMSampleBufferRef sampleBuffer = nullptr;
    bool isKeyFrame = false;

    while (!isKeyFrame) {
        if (d.reader.status == AVAssetReaderStatusCompleted) {
            d.error = core::Error(info().name, "end of range, no more samples to copy");
            qWarning() << "warning:" << d.error.message();
            return av::Time();
        }

        sampleBuffer = [d.videoOutput copyNextSampleBuffer];

        if (!sampleBuffer) {
            d.error = core::Error(info().name, "failed when trying to read sample buffer");
            qWarning() << "warning:" << d.error.message();
            return av::Time();
        }

        d.timeStamp = av::Time::convert(toTime(CMSampleBufferGetPresentationTimeStamp(sampleBuffer)), d.fps);

        isKeyFrame = true;

        CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true);

        if (attachments) {
            CFDictionaryRef attachment = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments, 0));
            CFBooleanRef notKeyFrame = static_cast<CFBooleanRef>(
                CFDictionaryGetValue(attachment, kCMSampleAttachmentKey_NotSync));

            if (notKeyFrame == kCFBooleanTrue)
                isKeyFrame = false;
        }

        if (!isKeyFrame)
            CFRelease(sampleBuffer);
    }

    CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

    if (!imageBuffer) {
        CFRelease(sampleBuffer);
        d.error = core::Error(info().name, "CMSampleBuffer has no image buffer");
        qWarning() << "warning:" << d.error.message();
        return av::Time();
    }

    CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

    if (!copyPixelBufferToImageBuffer(imageBuffer, d.image, d.error)) {
        CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
        CFRelease(sampleBuffer);
        qWarning() << "warning:" << d.error.message();
        return av::Time();
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

    if (!sampleBuffer)
        return {};

    d.timeStamp = av::Time::convert(toTime(CMSampleBufferGetPresentationTimeStamp(sampleBuffer)), d.fps);

    CFRelease(sampleBuffer);
    return d.timeStamp;
}

av::Time
QuicktimeReaderPrivate::seek(const av::TimeRange& timerange)
{
    FM_PROFILE_SCOPE("quicktime", "seek");
    core::PerfScope phase("quicktime.seek");
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
        d.error = core::Error(info().name, QString("failed to recreate AVAssetReader: %1")
                                               .arg(QString::fromNSString(error.localizedDescription)));
        qWarning() << "warning:" << d.error.message();
        return {};
    }

    if (!d.videoTrack) {
        d.error = core::Error(info().name, "no video track found in asset");
        qWarning() << "warning:" << d.error.message();
        return {};
    }

    d.videoOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:d.videoTrack outputSettings:videoOutputSettings()];

    if (!d.videoOutput) {
        d.error = core::Error(info().name, "unable to create AVAssetReaderTrackOutput");
        qWarning() << "warning:" << d.error.message();
        return {};
    }

    [d.reader addOutput:d.videoOutput];

    d.timeStamp = time;
    d.reader.timeRange = toTimerange(timerange);

    if (![d.reader startReading]) {
        d.error = core::Error(info().name, "failed to start reading after seeking");
        qWarning() << "warning:" << d.error.message();
        return {};
    }

    return d.timeStamp;
}

CMTime
QuicktimeReaderPrivate::toTime(const av::Time& other)
{
    return CMTimeMakeWithEpoch(other.ticks(), other.timeScale(), 0);
}

av::Time
QuicktimeReaderPrivate::toTime(const CMTime& other)
{
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
QuicktimeReaderPrivate::toTimeRange(const CMTimeRange& timeRange)
{
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
    static const QList<QString> extensions = { "mov", "mp4",  "m4v", "avi", "mkv", "flv",
                                               "mpg", "mpeg", "3gp", "3g2", "mxf", "wmv" };

    return extensions;
}

QuicktimeReader::QuicktimeReader(QObject* parent)
    : plugins::MediaReader(parent)
    , p(new QuicktimeReaderPrivate())
{
    p->d.object = this;
    p->init();
}

QuicktimeReader::~QuicktimeReader() { p->close(); }

bool
QuicktimeReader::open(const core::File& file, const Options& options)
{
    return p->open(file, options);
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

bool
QuicktimeReader::supportsConcurrent() const
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

core::MetaData
QuicktimeReader::metaData() const
{
    return p->d.metaData;
}

core::Error
QuicktimeReader::error() const
{
    if (dispatch_group_wait(p->d.loading, DISPATCH_TIME_NOW) != 0)
        return {};

    return p->d.error;
}

plugins::PluginHandler
QuicktimeReader::handler()
{
    static plugins::PluginHandler handler = plugins::PluginHandler::create<MediaReader>(
        QuicktimeReaderPrivate::info(), QuicktimeReaderPrivate::extensions, QuicktimeReaderPrivate::creator);

    return handler;
}

}  // namespace flipman::sdk::plugins
