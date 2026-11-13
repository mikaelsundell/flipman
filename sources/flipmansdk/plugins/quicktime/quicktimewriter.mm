// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "render/private/metalimageconverter.h"

#include <flipmansdk/core/filerange.h>
#include <flipmansdk/core/profile.h>
#include <flipmansdk/plugins/quicktime/quicktimewriter.h>
#include <AVFoundation/AVFoundation.h>
#include <CoreVideo/CoreVideo.h>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSemaphore>
#include <QThread>
#include <limits>
#include <memory>

namespace flipman::sdk::plugins {

namespace {

    QString message(NSError* error, const QString& fallback)
    {
        return error ? QString::fromUtf8(error.localizedDescription.UTF8String) : fallback;
    }

    AVVideoCodecType codec(const QString& name)
    {
        if (name == "h264")
            return AVVideoCodecTypeH264;

        if (name == "hevc")
            return AVVideoCodecTypeHEVC;

        if (name == "prores422")
            return AVVideoCodecTypeAppleProRes422;

        if (name == "prores422hq")
            return AVVideoCodecTypeAppleProRes422HQ;

        if (name == "prores422lt")
            return AVVideoCodecTypeAppleProRes422LT;

        if (name == "prores422proxy")
            return AVVideoCodecTypeAppleProRes422Proxy;

        if (name == "prores4444")
            return AVVideoCodecTypeAppleProRes4444;

        if (name == "prores4444xq")
            return AVVideoCodecTypeAppleProRes4444XQ;

        return nil;
    }

    // CoreVideo retains a shallow ImageBuffer handle, not a copied frame. The
    // caller must leave the shared pixel storage immutable until close() returns.
    void releaseBytes(void* context, const void*) { delete static_cast<core::ImageBuffer*>(context); }

    void releasePlanes(void* context, const void*, size_t, size_t, const void*[])
    {
        delete static_cast<core::ImageBuffer*>(context);
    }

    CVPixelBufferRef pixelBuffer(const core::ImageBuffer& image)
    {
        using Image = core::ImageBuffer;
        using Format = core::ImageFormat;
        if (!image.isAllocated() || image.dataWindow().isEmpty())
            return nullptr;

        const auto size = image.dataWindow().size();
        auto owner = std::make_unique<Image>(image);
        CVPixelBufferRef buffer = nullptr;
        CVReturn result = kCVReturnInvalidPixelFormat;
        if (image.packing() == Image::Packing::BiPlanar && image.pixelLayout() == Image::PixelLayout::NV12
            && image.subsampling() == Image::Subsampling::CS420 && image.imageFormat() == Format::UInt8
            && size.width() % 2 == 0 && size.height() % 2 == 0 && image.planeCount() == 2) {
            void* addresses[] = { image.planeData(0), image.planeData(1) };
            size_t widths[] = { size_t(size.width()), size_t(size.width() / 2) };
            size_t heights[] = { size_t(size.height()), size_t(size.height() / 2) };
            size_t strides[] = { image.planeStride(0), image.planeStride(1) };
            const OSType type = image.pixelRange() == Image::PixelRange::Full
                                    ? kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
                                    : kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
            result = CVPixelBufferCreateWithPlanarBytes(kCFAllocatorDefault, size.width(), size.height(), type, nullptr,
                                                        0, 2, addresses, widths, heights, strides, releasePlanes,
                                                        owner.get(), nullptr, &buffer);
        }
        else if (image.packing() == Image::Packing::Interleaved && image.channels() == 4
                 && image.subsampling() == Image::Subsampling::None) {
            OSType type = 0;
            if (image.pixelLayout() == Image::PixelLayout::BGRA && image.imageFormat() == Format::UInt8)
                type = kCVPixelFormatType_32BGRA;
            else if (image.pixelLayout() == Image::PixelLayout::RGBA) {
                if (image.imageFormat() == Format::UInt8)
                    type = kCVPixelFormatType_32RGBA;
                else if (image.imageFormat() == Format::Half)
                    type = kCVPixelFormatType_64RGBAHalf;
                else if (image.imageFormat() == Format::Float)
                    type = kCVPixelFormatType_128RGBAFloat;
            }
            if (type)
                result = CVPixelBufferCreateWithBytes(kCFAllocatorDefault, size.width(), size.height(), type,
                                                      image.data(), image.strideSize(), releaseBytes, owner.get(),
                                                      nullptr, &buffer);
        }
        if (result == kCVReturnSuccess)
            owner.release();
        return buffer;
    }

    NSDictionary* colorProperties(const core::ImageBuffer& image)
    {
        using namespace render;
        NSString* primaries = nil;
        NSString* matrix = nil;
        switch (image.colorSpace()) {
        case ColorSpace::Unknown:
        case ColorSpace::Rec709:
            primaries = AVVideoColorPrimaries_ITU_R_709_2;
            matrix = AVVideoYCbCrMatrix_ITU_R_709_2;
            break;
        case ColorSpace::Rec601:
            primaries = AVVideoColorPrimaries_SMPTE_C;
            matrix = AVVideoYCbCrMatrix_ITU_R_601_4;
            break;
        case ColorSpace::DisplayP3:
            primaries = AVVideoColorPrimaries_P3_D65;
            matrix = AVVideoYCbCrMatrix_ITU_R_709_2;
            break;
        case ColorSpace::Rec2020:
            primaries = AVVideoColorPrimaries_ITU_R_2020;
            matrix = AVVideoYCbCrMatrix_ITU_R_2020;
            break;
        default: return nil;
        }
        NSString* transfer = nil;
        switch (image.transferFunction()) {
        case TransferFunction::Unknown:
        case TransferFunction::Gamma24: transfer = AVVideoTransferFunction_ITU_R_709_2; break;
        case TransferFunction::SRGB: transfer = AVVideoTransferFunction_IEC_sRGB; break;
        case TransferFunction::Linear: transfer = AVVideoTransferFunction_Linear; break;
        default: return nil;
        }
        return @ {
            AVVideoColorPrimariesKey : primaries,
            AVVideoTransferFunctionKey : transfer,
            AVVideoYCbCrMatrixKey : matrix
        };
    }

}  // namespace

class QuicktimeWriterPrivate {
public:
    ~QuicktimeWriterPrivate();
    bool fail(const QString& text);
    bool start(const core::ImageBuffer& image, CVPixelBufferRef buffer);
    bool close();
    void clear();
    av::Time currentTime() const;
    CVPixelBufferRef rgbaBuffer(const core::ImageBuffer& image);

    struct Data {
        AVAssetWriter* writer = nil;
        AVAssetWriterInput* input = nil;
        CMVideoFormatDescriptionRef formatDescription = nullptr;
        NSDictionary* colors = nil;
        OSType pixelFormat = 0;
        core::Error error;
        core::MetaData metadata;
        core::File file;
        core::ImageBuffer::PixelLayout sourceLayout = core::ImageBuffer::PixelLayout::Unknown;
        av::Fps fps = av::Fps::fps24();
        av::TimeRange range;
        render::ColorSpace gamut = render::ColorSpace::Unknown;
        render::TransferFunction transfer = render::TransferFunction::Unknown;
        mutable QMutex mutex;
        QString codec;
        QSize size;
        std::unique_ptr<render::MetalImageConverter> packingConverter;
        qint64 frames = 0;
        int bitrate = 0;
        int timeoutMs = 30000;
        bool open = false;
        bool started = false;
    };
    Data d;
};

QuicktimeWriterPrivate::~QuicktimeWriterPrivate()
{
    @autoreleasepool {
        close();
    }
}

bool
QuicktimeWriterPrivate::fail(const QString& text)
{
    d.error = core::Error("quicktimewriter", text);
    return false;
}

av::Time
QuicktimeWriterPrivate::currentTime() const
{
    const qint64 start = d.range.isValid() ? av::Time::convert(d.range.start(), d.fps).frames() : 0;
    return av::Time::fromFrames(start + d.frames, d.fps);
}

void
QuicktimeWriterPrivate::clear()
{
    d.packingConverter.reset();
    [d.colors release];
    d.colors = nil;
    if (d.formatDescription)
        CFRelease(d.formatDescription);
    d.formatDescription = nullptr;
    [d.input release];
    [d.writer release];
    d.input = nil;
    d.writer = nil;
    d.open = false;
    d.started = false;
}

CVPixelBufferRef
QuicktimeWriterPrivate::rgbaBuffer(const core::ImageBuffer& image)
{
    if (!image.isAllocated() || image.imageFormat() != core::ImageFormat::UInt8 || image.channels() != 4
        || image.packing() != core::ImageBuffer::Packing::Interleaved
        || image.pixelLayout() != core::ImageBuffer::PixelLayout::RGBA
        || image.subsampling() != core::ImageBuffer::Subsampling::None)
        return nullptr;

    if (!d.packingConverter)
        d.packingConverter = std::make_unique<render::MetalImageConverter>();
    auto buffer = d.packingConverter->convert(image);
    if (!buffer)
        d.error = d.packingConverter->error();
    return buffer;
}

bool
QuicktimeWriterPrivate::start(const core::ImageBuffer& image, CVPixelBufferRef buffer)
{
    NSDictionary* colors = colorProperties(image);
    if (!colors)
        return fail("unsupported color tags; apply the intended GPU output transform before writing");

    d.size = image.dataWindow().size();
    NSMutableDictionary* settings = [NSMutableDictionary dictionaryWithDictionary:@{
        AVVideoCodecKey : codec(d.codec),
        AVVideoWidthKey : @(d.size.width()),
        AVVideoHeightKey : @(d.size.height()),
        AVVideoColorPropertiesKey : colors
    }];
    NSMutableDictionary* compression = [NSMutableDictionary dictionaryWithDictionary:@{
        AVVideoExpectedSourceFrameRateKey : @(d.fps.real())
    }];
    if (d.bitrate)
        compression[AVVideoAverageBitRateKey] = @(d.bitrate);
    if (!d.codec.startsWith("prores"))
        settings[AVVideoCompressionPropertiesKey] = compression;
    if (![d.writer canApplyOutputSettings:settings forMediaType:AVMediaTypeVideo])
        return fail(QString("codec/settings are not supported by this container or encoder: %1").arg(d.codec));

    d.input = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:settings];
    d.input.expectsMediaDataInRealTime = NO;
    d.input.mediaTimeScale = int32_t(d.fps.numerator());
    if (![d.writer canAddInput:d.input])
        return fail("video input cannot be added to this container");

    [d.writer addInput:d.input];
    NSMutableArray* metadata = [NSMutableArray array];
    for (const auto& entry : QList<QPair<QString, NSString*>> { { "title", AVMetadataCommonKeyTitle },
                                                                { "author", AVMetadataCommonKeyCreator },
                                                                { "description", AVMetadataCommonKeyDescription } }) {
        QVariant value = d.metadata.value(entry.first);
        if (!value.isValid())
            value = d.metadata.value(core::MetaData::Group::Container, entry.first);
        if (!value.isValid() && entry.first == "title")
            value = d.metadata.value(core::MetaData::convert(core::MetaData::Title));
        if (value.isValid()) {
            AVMutableMetadataItem* item = [AVMutableMetadataItem metadataItem];
            item.keySpace = AVMetadataKeySpaceCommon;
            item.key = entry.second;
            item.value = [NSString stringWithUTF8String:value.toString().toUtf8().constData()];
            [metadata addObject:item];
        }
    }
    d.writer.metadata = metadata;
    if (![d.writer startWriting])
        return fail(message(d.writer.error, "could not start video encoder"));

    [d.writer startSessionAtSourceTime:kCMTimeZero];
    d.started = true;
    d.colors = [colors retain];
    d.pixelFormat = CVPixelBufferGetPixelFormatType(buffer);
    d.sourceLayout = image.pixelLayout();
    d.gamut = image.colorSpace();
    d.transfer = image.transferFunction();
    return true;
}

bool
QuicktimeWriterPrivate::close()
{
    if (!d.open)
        return !d.error.hasError();

    FM_PROFILE_SCOPE("quicktimewriter", "finalize");
    bool ok = true;
    if (!d.started || !d.frames) {
        [d.writer cancelWriting];
        ok = fail("cannot finalize a movie without video frames");
    }
    else if (d.writer.status != AVAssetWriterStatusWriting) {
        ok = fail(message(d.writer.error, "video writer is no longer writing"));
        [d.writer cancelWriting];
    }
    else {
        [d.input markAsFinished];
        [d.writer endSessionAtSourceTime:CMTimeMake(d.frames * d.fps.denominator(), int32_t(d.fps.numerator()))];
        auto completion = std::make_shared<QSemaphore>();
        AVAssetWriter* finishingWriter = [d.writer retain];
        [d.writer finishWritingWithCompletionHandler:^{
            completion->release();
            [finishingWriter release];
        }];
        if (!completion->tryAcquire(1, d.timeoutMs)) {
            [d.writer cancelWriting];
            ok = fail("timed out finalizing the movie");
        }
        else if (d.writer.status != AVAssetWriterStatusCompleted)
            ok = fail(message(d.writer.error, "could not finalize movie"));
    }
    if (ok)
        d.error = {};
    clear();
    return ok;
}

QuicktimeWriter::QuicktimeWriter(QObject* parent)
    : MediaWriter(parent)
    , p(new QuicktimeWriterPrivate)
{}

QuicktimeWriter::~QuicktimeWriter() = default;

bool
QuicktimeWriter::open(const core::File& file, const Options& options)
{
    QMutexLocker lock(&p->d.mutex);
    @autoreleasepool {
        if (thread() != QThread::currentThread())
            return p->fail("open the writer on its owning thread");

        if (p->d.open)
            return p->fail("close the current movie before opening another");

        p->d.error = {};
        const QString ext = file.extension().toLower();
        if (!extensions().contains(ext))
            return p->fail("unsupported movie extension");

        if (file.fileRange().size() > 0 || file.filePath().contains('#') || file.filePath().isEmpty()
            || QFileInfo::exists(file.filePath()))
            return p->fail("target must be a new, non-sequence file; existing files are never overwritten");

        for (auto it = options.values.cbegin(); it != options.values.cend(); ++it)
            if (it.key() != "codec" && it.key() != "bitrate" && it.key() != "timeoutMs")
                return p->fail("unknown writer option: " + it.key());

        p->d.codec = options.values.value("codec", ext == "mov" ? "prores422" : "h264").toString().toLower();
        if (!codec(p->d.codec) || (ext != "mov" && p->d.codec.startsWith("prores")))
            return p->fail("unsupported codec/container combination");

        p->d.bitrate = 0;
        p->d.timeoutMs = 30000;
        for (const QString key : { QStringLiteral("bitrate"), QStringLiteral("timeoutMs") }) {
            if (!options.values.contains(key))
                continue;
            bool valid = false;
            const QVariant value = options.values.value(key);
            const qlonglong number = value.toLongLong(&valid);
            if (!valid || value.typeId() == QMetaType::Bool || value.typeId() == QMetaType::QString
                || value.toDouble() != double(number) || number < 1 || number > std::numeric_limits<int>::max()
                || (key == "timeoutMs" && number > 120000))
                return p->fail("invalid writer option: " + key);

            if (key == "bitrate")
                p->d.bitrate = int(number);
            else
                p->d.timeoutMs = int(number);
        }
        if (p->d.bitrate && p->d.codec.startsWith("prores"))
            return p->fail("bitrate is only supported for H.264/HEVC");

        NSError* error = nil;
        NSString* path = [NSString stringWithUTF8String:file.filePath().toUtf8().constData()];
        p->d.writer = [[AVAssetWriter alloc] initWithURL:[NSURL fileURLWithPath:path]
                                                fileType:ext == "mov" ? AVFileTypeQuickTimeMovie : AVFileTypeMPEG4
                                                   error:&error];
        if (!p->d.writer)
            return p->fail(message(error, "could not open movie output"));

        p->d.frames = 0;
        p->d.file = file;
        p->d.open = true;
        return true;
    }
}

bool
QuicktimeWriter::close()
{
    QMutexLocker lock(&p->d.mutex);
    @autoreleasepool {
        if (thread() != QThread::currentThread())
            return p->fail("close the writer on its owning thread");

        return p->close();
    }
}

bool
QuicktimeWriter::isOpen() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.open;
}

bool
QuicktimeWriter::supportsImage() const
{
    return true;
}

bool
QuicktimeWriter::supportsAudio() const
{
    return false;
}

QList<QString>
QuicktimeWriter::extensions() const
{
    return { "mov", "mp4" };
}

av::Time
QuicktimeWriter::write(const core::AudioBuffer&)
{
    QMutexLocker lock(&p->d.mutex);
    p->fail("audio writing is not supported by the current AudioBuffer API");
    return {};
}

av::Time
QuicktimeWriter::write(const core::ImageBuffer& image)
{
    FM_PROFILE_SCOPE("quicktimewriter", "append");
    QMutexLocker lock(&p->d.mutex);
    @autoreleasepool {
        p->d.error = {};
        if (thread() != QThread::currentThread()) {
            p->fail("write frames on the writer's owning thread");
            return {};
        }
        if (!p->d.open) {
            p->fail("writer is not open");
            return {};
        }
        if (p->d.range.isValid() && p->d.frames >= av::Time::convert(p->d.range.duration(), p->d.fps).frames()) {
            p->fail("configured frame range is complete");
            return {};
        }
        if (p->d.started && (image.dataWindow().size() != p->d.size || image.pixelLayout() != p->d.sourceLayout)) {
            p->fail("frame dimensions and source layout must remain constant within a movie");
            return {};
        }
        CVPixelBufferRef buffer = pixelBuffer(image);
        if (!buffer)
            buffer = p->rgbaBuffer(image);
        if (!buffer) {
            if (!p->d.error.hasError())
                p->fail("unsupported pixels; use NV12 UInt8, BGRA UInt8 or RGBA UInt8/Half/Float");
            return {};
        }
        if (p->d.started
            && (image.dataWindow().size() != p->d.size || CVPixelBufferGetPixelFormatType(buffer) != p->d.pixelFormat
                || image.colorSpace() != p->d.gamut || image.transferFunction() != p->d.transfer)) {
            CVPixelBufferRelease(buffer);
            p->fail("frame dimensions, layout and color tags must remain constant within a movie");
            return {};
        }
        if (!p->d.started && !p->start(image, buffer)) {
            CVPixelBufferRelease(buffer);
            [p->d.writer cancelWriting];
            p->clear();
            return {};
        }
        QElapsedTimer wait;
        wait.start();
        while (!p->d.input.readyForMoreMediaData && p->d.writer.status == AVAssetWriterStatusWriting
               && wait.elapsed() < p->d.timeoutMs)
            QThread::msleep(1);
        if (!p->d.input.readyForMoreMediaData || p->d.writer.status != AVAssetWriterStatusWriting) {
            CVPixelBufferRelease(buffer);
            p->fail(message(p->d.writer.error, "timed out waiting for video encoder"));
            return {};
        }
        CVBufferSetAttachment(buffer, kCVImageBufferColorPrimariesKey,
                              (CFStringRef)p->d.colors[AVVideoColorPrimariesKey], kCVAttachmentMode_ShouldPropagate);
        CVBufferSetAttachment(buffer, kCVImageBufferTransferFunctionKey,
                              (CFStringRef)p->d.colors[AVVideoTransferFunctionKey], kCVAttachmentMode_ShouldPropagate);
        CVBufferSetAttachment(buffer, kCVImageBufferYCbCrMatrixKey, (CFStringRef)p->d.colors[AVVideoYCbCrMatrixKey],
                              kCVAttachmentMode_ShouldPropagate);
        const CMTime pts = CMTimeMake(p->d.frames * p->d.fps.denominator(), int32_t(p->d.fps.numerator()));

        // Explicit sample durations preserve the requested rate even for a
        // single-frame ProRes file; an adaptor only supplies presentation time.
        OSStatus status = noErr;
        if (!p->d.formatDescription)
            status = CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, buffer, &p->d.formatDescription);
        CMSampleBufferRef sample = nullptr;
        const CMSampleTimingInfo timing = { CMTimeMake(p->d.fps.denominator(), int32_t(p->d.fps.numerator())), pts,
                                            kCMTimeInvalid };
        if (status == noErr)
            status = CMSampleBufferCreateReadyWithImageBuffer(kCFAllocatorDefault, buffer, p->d.formatDescription,
                                                              &timing, &sample);
        const bool ok = status == noErr && [p->d.input appendSampleBuffer:sample];
        if (sample)
            CFRelease(sample);
        CVPixelBufferRelease(buffer);
        if (!ok) {
            p->fail(message(p->d.writer.error, "could not append video frame"));
            return {};
        }
        const auto result = p->currentTime();
        ++p->d.frames;
        p->d.error = {};
        return result;
    }
}

av::Time
QuicktimeWriter::seek(const av::TimeRange& range)
{
    QMutexLocker lock(&p->d.mutex);
    if (p->d.started || !range.isValid() || range.start().fps().numerator() <= 0
        || range.duration().fps().numerator() <= 0 || range.start().frames() < 0
        || av::Time::convert(range.duration(), p->d.fps).frames() < 1) {
        p->fail("set a valid range before the first frame; movie writing is sequential");
        return {};
    }
    p->d.range = range;
    p->d.error = {};
    return p->currentTime();
}

av::Time
QuicktimeWriter::time() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->currentTime();
}

av::Fps
QuicktimeWriter::fps() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.fps;
}

av::TimeRange
QuicktimeWriter::timeRange() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.range.isValid() ? p->d.range
                                : av::TimeRange(av::Time::zero(p->d.fps), av::Time::fromFrames(p->d.frames, p->d.fps));
}

void
QuicktimeWriter::setFps(const av::Fps& fps)
{
    QMutexLocker lock(&p->d.mutex);
    if (p->d.started || !fps.isValid() || fps.numerator() <= 0
        || fps.numerator() > std::numeric_limits<int32_t>::max()) {
        p->fail("set a valid frame rate before the first frame");
        return;
    }
    p->d.fps = fps;
    p->d.error = {};
}

void
QuicktimeWriter::setTimeRange(const av::TimeRange& range)
{
    seek(range);
}

bool
QuicktimeWriter::setMetaData(const core::MetaData& metadata)
{
    QMutexLocker lock(&p->d.mutex);
    if (p->d.started)
        return p->fail("set metadata before the first frame");

    p->d.metadata = metadata;
    p->d.error = {};
    return true;
}

core::Error
QuicktimeWriter::error() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.error;
}

core::Plugin*
QuicktimeWriter::creator()
{
    return new QuicktimeWriter;
}

PluginHandler
QuicktimeWriter::handler()
{
    return PluginHandler::create<MediaWriter>(
        { "quicktimewriter", "writes video using AVFoundation", "1.0.0" },
        [] { return QList<QString> { "mov", "mp4" }; }, QuicktimeWriter::creator);
}

}  // namespace flipman::sdk::plugins
