// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-present Mikael Sundell
// Decode/callback flow adapted from github.com/mikaelsundell/brawtool/brawtool.cpp.

#include <flipmansdk/core/log.h>
#include <flipmansdk/core/profile.h>
#include <flipmansdk/plugins/braw/brawreader.h>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
#include <mutex>
#include <utility>
#ifdef FLIPMAN_HAS_BRAW
#    include <BlackmagicRawAPI.h>
#endif

namespace flipman::sdk::plugins {

namespace {

#ifdef FLIPMAN_HAS_BRAW
    template<class T> void release(T*& object)
    {
        if (object)
            object->Release();
        object = nullptr;
    }

    CFStringRef cfString(const QString& string)
    {
        const auto utf8 = string.toUtf8();
        return CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(utf8.constData()),
                                       utf8.size(), kCFStringEncodingUTF8, false);
    }

    QString qtString(CFStringRef string)
    {
        if (!string)
            return {};
        const auto length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(string), kCFStringEncodingUTF8) + 1;
        QByteArray bytes(length, '\0');
        return CFStringGetCString(string, bytes.data(), bytes.size(), kCFStringEncodingUTF8)
                   ? QString::fromUtf8(bytes.constData())
                   : QString();
    }

    void metadata(IBlackmagicRawMetadataIterator* iterator, core::MetaData& output)
    {
        if (!iterator)
            return;

        CFStringRef key = nullptr;
        while (iterator->GetKey(&key) == S_OK) {
            Variant value;
            VariantInit(&value);
            if (iterator->GetData(&value) != S_OK) {
                VariantClear(&value);
                break;
            }
            QVariant data;
            switch (value.vt) {
            case blackmagicRawVariantTypeS16: data = value.iVal; break;
            case blackmagicRawVariantTypeU16: data = value.uiVal; break;
            case blackmagicRawVariantTypeS32: data = value.intVal; break;
            case blackmagicRawVariantTypeU32: data = value.uintVal; break;
            case blackmagicRawVariantTypeFloat32: data = value.fltVal; break;
            case blackmagicRawVariantTypeString: data = qtString(value.bstrVal); break;
            case blackmagicRawVariantTypeSafeArray: {
                BlackmagicRawVariantType type;
                long lower = 0, upper = -1;
                void* elements = nullptr;
                auto* array = value.parray;
                if (array && SafeArrayGetVartype(array, &type) == S_OK && SafeArrayGetLBound(array, 1, &lower) == S_OK
                    && SafeArrayGetUBound(array, 1, &upper) == S_OK && upper >= lower && upper - lower < 1000000
                    && SafeArrayAccessData(array, &elements) == S_OK) {
                    QVariantList list;
                    if (elements)
                        for (long i = 0; i <= upper - lower; ++i) {
                            switch (type) {
                            case blackmagicRawVariantTypeU8: list.append(static_cast<uint8_t*>(elements)[i]); break;
                            case blackmagicRawVariantTypeS16: list.append(static_cast<int16_t*>(elements)[i]); break;
                            case blackmagicRawVariantTypeU16: list.append(static_cast<uint16_t*>(elements)[i]); break;
                            case blackmagicRawVariantTypeS32: list.append(static_cast<int32_t*>(elements)[i]); break;
                            case blackmagicRawVariantTypeU32: list.append(static_cast<uint32_t*>(elements)[i]); break;
                            case blackmagicRawVariantTypeFloat32: list.append(static_cast<float*>(elements)[i]); break;
                            default: break;
                            }
                        }
                    SafeArrayUnaccessData(array);
                    if (!list.isEmpty())
                        data = list;
                }
                break;
            }
            default: break;
            }
            if (data.isValid())
                output.insert(core::MetaData::Group::Camera, qtString(key), data);
            VariantClear(&value);
            if (iterator->Next() != S_OK)
                break;
        }
    }

    class BrawCallback final : public IBlackmagicRawCallback {
    public:
        std::atomic<ULONG> references { 1 };
        std::mutex mutex;
        QString error;
        core::ImageBuffer image;
        core::MetaData frameMetadata;
        QVariantMap options;  // Immutable while jobs are in flight.
        IBlackmagicRawClipProcessingAttributes* clipAttributes = nullptr;

        ~BrawCallback() override { release(clipAttributes); }

        bool completed = false;

        HRESULT QueryInterface(REFIID, LPVOID* output) override
        {
            if (output)
                *output = nullptr;
            return E_NOINTERFACE;
        }

        ULONG AddRef() override { return ++references; }

        ULONG Release() override
        {
            const auto count = --references;
            if (!count)
                delete this;
            return count;
        }

        void fail(const QString& operation, HRESULT result)
        {
            std::lock_guard<std::mutex> guard(mutex);
            if (error.isEmpty())
                error = QString("%1 failed (0x%2)").arg(operation).arg(quint32(result), 8, 16, QLatin1Char('0'));
        }

        void ReadComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawFrame* frame) override
        {
            IBlackmagicRawJob* decode = nullptr;
            IBlackmagicRawFrameProcessingAttributes* attributes = nullptr;
            if (result == S_OK && !frame)
                result = E_FAIL;
            if (result == S_OK)
                result = frame->SetResourceFormat(blackmagicRawResourceFormatRGBF32);
            if (result == S_OK && !options.isEmpty())
                result = frame->CloneFrameProcessingAttributes(&attributes);
            if (result == S_OK && attributes) {
                for (auto it = options.cbegin(); it != options.cend() && result == S_OK; ++it) {
                    Variant value;
                    VariantInit(&value);
                    BlackmagicRawFrameProcessingAttribute attribute;
                    if (it.key() == "kelvin") {
                        value.vt = blackmagicRawVariantTypeU32;
                        value.uintVal = it.value().toUInt();
                        attribute = blackmagicRawFrameProcessingAttributeWhiteBalanceKelvin;
                    }
                    else if (it.key() == "tint") {
                        value.vt = blackmagicRawVariantTypeS16;
                        value.iVal = short(it.value().toInt());
                        attribute = blackmagicRawFrameProcessingAttributeWhiteBalanceTint;
                    }
                    else {
                        value.vt = blackmagicRawVariantTypeFloat32;
                        value.fltVal = it.value().toFloat();
                        attribute = blackmagicRawFrameProcessingAttributeExposure;
                    }
                    result = attributes->SetFrameAttribute(attribute, &value);
                    VariantClear(&value);
                }
            }
            if (result == S_OK) {
                IBlackmagicRawMetadataIterator* iterator = nullptr;
                if (frame->GetMetadataIterator(&iterator) == S_OK) {
                    // Only this callback writes frame metadata before process submission.
                    metadata(iterator, frameMetadata);
                    release(iterator);
                }
                result = frame->CreateJobDecodeAndProcessFrame(clipAttributes, attributes, &decode);
            }
            release(attributes);
            if (result == S_OK)
                result = decode->Submit();
            if (result != S_OK) {
                release(decode);
                fail("read/decode submission", result);
            }
            job->Release();
        }

        void ProcessComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawProcessedImage* processed) override
        {
            try {
                uint32_t width = 0, height = 0, bytes = 0;
                void* pixels = nullptr;
                BlackmagicRawResourceType type {};
                BlackmagicRawResourceFormat format {};
                if (result == S_OK && !processed)
                    result = E_FAIL;
                if (result == S_OK)
                    result = processed->GetResourceType(&type);
                if (result == S_OK && type != blackmagicRawResourceTypeBufferCPU)
                    result = E_FAIL;
                if (result == S_OK)
                    result = processed->GetResourceFormat(&format);
                if (result == S_OK && format != blackmagicRawResourceFormatRGBF32)
                    result = E_FAIL;
                if (result == S_OK)
                    result = processed->GetWidth(&width);
                if (result == S_OK)
                    result = processed->GetHeight(&height);
                if (result == S_OK)
                    result = processed->GetResourceSizeBytes(&bytes);
                if (result == S_OK)
                    result = processed->GetResource(&pixels);
                qCDebug(core::mediaLog) << "BRAW processed resource" << type << format << width << height << bytes
                                        << result;
                const quint64 expected = quint64(width) * height * 3 * sizeof(float);
                if (result == S_OK
                    && (!pixels || !width || !height || width > INT_MAX || height > INT_MAX || expected > bytes))
                    result = E_FAIL;
                if (result == S_OK) {
                    core::ImageBuffer output(QRect(0, 0, int(width), int(height)), QRect(0, 0, int(width), int(height)),
                                             core::ImageFormat(core::ImageFormat::Float), 3);
                    output.setPixelLayout(core::ImageBuffer::PixelLayout::RGB);
                    output.allocate();
                    qCDebug(core::mediaLog) << "BRAW image allocation" << output.byteSize() << bytes;
                    if (!output.isAllocated() || output.byteSize() != expected)
                        result = E_FAIL;
                    else {
                        std::memcpy(output.data(), pixels, size_t(expected));  // Own pixels beyond callback lifetime.
                        std::lock_guard<std::mutex> guard(mutex);
                        image = output;
                        completed = true;
                    }
                }
                if (result != S_OK)
                    fail("process frame", result);
            } catch (const std::exception& exception) {
                fail(QString("process frame: %1").arg(QString::fromUtf8(exception.what())), E_FAIL);
            }
            job->Release();
        }

        void DecodeComplete(IBlackmagicRawJob*, HRESULT) override {}

        void TrimProgress(IBlackmagicRawJob*, float) override {}

        void TrimComplete(IBlackmagicRawJob*, HRESULT) override {}

        void SidecarMetadataParseWarning(IBlackmagicRawClip*, CFStringRef, uint32_t line, CFStringRef info) override
        {
            qCWarning(core::mediaLog) << "BRAW sidecar line" << line << qtString(info);
        }

        void SidecarMetadataParseError(IBlackmagicRawClip*, CFStringRef, uint32_t line, CFStringRef info) override
        {
            qCWarning(core::mediaLog) << "BRAW sidecar ignored, line" << line << qtString(info);
        }

        void PreparePipelineComplete(void*, HRESULT) override {}
    };
#endif

}  // namespace

class BrawReaderPrivate {
public:
    ~BrawReaderPrivate() { close(); }

    bool close()
    {
#ifdef FLIPMAN_HAS_BRAW
        if (d.codec) {
            d.codec->FlushJobs();
            d.codec->SetCallback(nullptr);
        }
        release(d.clip);
        release(d.codec);
        release(d.factory);
        release(d.callback);
#endif
        d.open = false;
        d.next = d.end = 0;
        d.image = {};
        d.clipMetadata = {};
        d.currentMetadata = {};
        d.range = {};
        d.fps = {};
        return true;
    }

    av::Time read()
    {
        FM_PROFILE_SCOPE("braw", "read");
        d.image = {};
        if (!d.open) {
            d.error = core::Error("brawreader", "reader is not open");
            return {};
        }
        if (d.error.hasError() || d.next >= d.end)
            return {};
#ifdef FLIPMAN_HAS_BRAW
        {
            std::lock_guard<std::mutex> guard(d.callback->mutex);
            d.callback->error.clear();
            d.callback->image = {};
            d.callback->frameMetadata = {};
            d.callback->completed = false;
        }
        IBlackmagicRawJob* job = nullptr;
        HRESULT result = d.clip->CreateJobReadFrame(uint64_t(d.next), &job);
        if (result == S_OK)
            result = job->Submit();
        if (result != S_OK)
            release(job);  // Successful jobs are released by callbacks.
        const HRESULT flushed = d.codec->FlushJobs();
        std::lock_guard<std::mutex> guard(d.callback->mutex);
        if (result != S_OK || flushed != S_OK || !d.callback->error.isEmpty() || !d.callback->completed) {
            d.error = core::Error("brawreader", d.callback->error.isEmpty() ? "frame read/flush did not complete"
                                                                            : d.callback->error);
            return {};
        }
        d.image = d.callback->image;
        d.currentMetadata = d.clipMetadata;
        for (const auto& key : d.callback->frameMetadata.keys(core::MetaData::Group::Camera))
            d.currentMetadata.insert(core::MetaData::Group::Camera, key,
                                     d.callback->frameMetadata.value(core::MetaData::Group::Camera, key));
        CFStringRef timecode = nullptr;
        if (d.clip->GetTimecodeForFrame(uint64_t(d.next), &timecode) == S_OK && timecode) {
            d.currentMetadata.insert(core::MetaData::Group::Timecode, "timecode", qtString(timecode));
            CFRelease(timecode);
        }
        return av::Time::fromFrames(d.next++, d.fps);
#else
        return {};
#endif
    }

    struct Data {
#ifdef FLIPMAN_HAS_BRAW
        IBlackmagicRawFactory* factory = nullptr;
        IBlackmagicRaw* codec = nullptr;
        IBlackmagicRawClip* clip = nullptr;
        BrawCallback* callback = nullptr;
#endif
        core::Error error;
        core::ImageBuffer image;
        core::MetaData clipMetadata, currentMetadata;
        av::Fps fps;
        av::TimeRange range;
        mutable QMutex mutex;
        qint64 next = 0, end = 0;
        bool open = false;
    };
    Data d;
};

BrawReader::BrawReader(QObject* parent)
    : MediaReader(parent)
    , p(new BrawReaderPrivate)
{}

BrawReader::~BrawReader() = default;

bool
BrawReader::open(const core::File& file, const Options& options)
{
    FM_PROFILE_SCOPE("braw", "open");
    QMutexLocker lock(&p->d.mutex);
    p->close();
    p->d.error = {};
#ifndef FLIPMAN_HAS_BRAW
    Q_UNUSED(file);
    Q_UNUSED(options);
    p->d.error = core::Error("brawreader", "Blackmagic RAW support was disabled at build time");
    return false;
#else
    if (!file.exists()) {
        p->d.error = core::Error("brawreader", "file does not exist");
        return false;
    }
    for (auto it = options.values.cbegin(); it != options.values.cend(); ++it) {
        bool valid = false;
        const double value = it.value().toDouble(&valid);
        const bool integral = std::floor(value) == value;
        if (!valid || it.value().typeId() == QMetaType::Bool || it.value().typeId() == QMetaType::QString
            || !std::isfinite(value)
            || !(it.key() == "kelvin" ? integral && value >= 2000 && value <= 50000
                 : it.key() == "tint" ? integral && value >= -150 && value <= 150
                                      : it.key() == "exposure" && value >= -20 && value <= 20)) {
            p->d.error = core::Error("brawreader", "invalid reader option: " + it.key());
            return false;
        }
    }
    p->d.factory = CreateBlackmagicRawFactoryInstance();
    QString runtime = qEnvironmentVariable("FLIPMAN_BRAW_RUNTIME_DIR");
    if (runtime.isEmpty()) {
        const QString root = qEnvironmentVariable("BLACKMAGIC_RAW_ROOT");
        if (!root.isEmpty())
            runtime = QDir(root).filePath(QFileInfo(QDir(root).filePath("Mac/Libraries")).isDir() ? "Mac/Libraries"
                                                                                                  : "Libraries");
    }
    if (!p->d.factory && !runtime.isEmpty()) {
        CFStringRef path = cfString(runtime);
        p->d.factory = CreateBlackmagicRawFactoryInstanceFromPath(path);
        CFRelease(path);
    }
    if (!p->d.factory) {
        p->d.error = core::Error(
            "brawreader",
            "Blackmagic RAW runtime not found in app Frameworks; set FLIPMAN_BRAW_RUNTIME_DIR to its framework parent");
        return false;
    }
    QString operation = "create codec";
    HRESULT result = p->d.factory->CreateCodec(&p->d.codec);
    IBlackmagicRawConfiguration* configuration = nullptr;
    if (result == S_OK) {
        operation = "query configuration";
        result = p->d.codec->QueryInterface(IID_IBlackmagicRawConfiguration, reinterpret_cast<void**>(&configuration));
    }
    if (result == S_OK) {
        operation = "select CPU pipeline";
        result = configuration->SetPipeline(blackmagicRawPipelineCPU, nullptr, nullptr);
    }
    release(configuration);
    if (result == S_OK) {
        p->d.callback = new BrawCallback;
        p->d.callback->options = options.values;
        operation = "set callback";
        result = p->d.codec->SetCallback(p->d.callback);
    }
    if (result == S_OK) {
        CFStringRef path = cfString(file.filePath());
        operation = "open clip";
        result = p->d.codec->OpenClip(path, &p->d.clip);
        CFRelease(path);
    }
    if (result == S_OK) {
        operation = "clone clip processing attributes";
        result = p->d.clip->CloneClipProcessingAttributes(&p->d.callback->clipAttributes);
    }
    if (result == S_OK) {
        // Display/preview LUTs belong to the render pipeline, not decoded pixels.
        Variant value;
        VariantInit(&value);
        value.vt = blackmagicRawVariantTypeString;
        value.bstrVal = cfString("Disabled");
        operation = "disable post-decode LUT";
        result = p->d.callback->clipAttributes->SetClipAttribute(blackmagicRawClipProcessingAttributePost3DLUTMode,
                                                                 &value);
        VariantClear(&value);
    }
    uint32_t width = 0, height = 0;
    uint64_t count = 0;
    float rate = 0;
    if (result == S_OK) {
        operation = "read clip dimensions";
        result = p->d.clip->GetWidth(&width);
    }
    if (result == S_OK)
        result = p->d.clip->GetHeight(&height);
    if (result == S_OK) {
        operation = "read frame rate";
        result = p->d.clip->GetFrameRate(&rate);
    }
    if (result == S_OK) {
        operation = "read frame count";
        result = p->d.clip->GetFrameCount(&count);
    }
    if (result != S_OK || !width || !height || !std::isfinite(rate) || rate <= 0 || !count
        || count > uint64_t(std::numeric_limits<qint64>::max() / 24000)) {
        p->d.error = core::Error("brawreader", QString("%1 failed or invalid clip properties (0x%2)")
                                                   .arg(operation)
                                                   .arg(quint32(result), 8, 16, QLatin1Char('0')));
        p->close();
        return false;
    }
    p->d.fps = av::Fps::guess(rate);
    if (!p->d.fps.isValid()) {
        p->d.error = core::Error("brawreader", "unsupported frame rate");
        p->close();
        return false;
    }
    p->d.end = qint64(count);
    p->d.range = av::TimeRange(av::Time::zero(p->d.fps), av::Time::fromFrames(p->d.end, p->d.fps));
    IBlackmagicRawMetadataIterator* iterator = nullptr;
    if (p->d.clip->GetMetadataIterator(&iterator) == S_OK) {
        metadata(iterator, p->d.clipMetadata);
        release(iterator);
    }
    p->d.clipMetadata.insert(core::MetaData::Group::Video, "width", width);
    p->d.clipMetadata.insert(core::MetaData::Group::Video, "height", height);
    p->d.clipMetadata.insert(core::MetaData::Group::Video, "frameCount", qulonglong(count));
    for (const auto& entry : { std::pair<BlackmagicRawClipProcessingAttribute, QString> {
                                   blackmagicRawClipProcessingAttributeGamma, "decodeGamma" },
                               { blackmagicRawClipProcessingAttributeGamut, "decodeGamut" } }) {
        Variant value;
        VariantInit(&value);
        if (p->d.callback->clipAttributes->GetClipAttribute(entry.first, &value) == S_OK
            && value.vt == blackmagicRawVariantTypeString)
            p->d.clipMetadata.insert(core::MetaData::Group::Video, entry.second, qtString(value.bstrVal));
        VariantClear(&value);
    }
    Variant generation;
    VariantInit(&generation);
    if (p->d.callback->clipAttributes->GetClipAttribute(blackmagicRawClipProcessingAttributeColorScienceGen, &generation)
            == S_OK
        && generation.vt == blackmagicRawVariantTypeU16)
        p->d.clipMetadata.insert(core::MetaData::Group::Video, "decodeColorScienceGen", generation.uiVal);
    VariantClear(&generation);
    p->d.currentMetadata = p->d.clipMetadata;
    p->d.open = true;
    qCDebug(core::mediaLog) << "BRAW opened" << file.filePath() << width << height << rate << count;
    return true;  // Media's synchronous-open path emits its readiness signal.
#endif
}

bool
BrawReader::close()
{
    QMutexLocker lock(&p->d.mutex);
    p->close();
    p->d.error = {};
    return true;
}

bool
BrawReader::isOpen() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.open;
}

bool
BrawReader::supportsImage() const
{
    return true;
}

bool
BrawReader::supportsAudio() const
{
    return false;
}

bool
BrawReader::supportsConcurrent() const
{
    return true;
}

QList<QString>
BrawReader::extensions() const
{
    return { "braw" };
}

av::Time
BrawReader::read()
{
    QMutexLocker lock(&p->d.mutex);
    return p->read();
}

av::Time
BrawReader::skip()
{
    QMutexLocker lock(&p->d.mutex);
    p->d.image = {};
    if (!p->d.open) {
        p->d.error = core::Error("brawreader", "reader is not open");
        return {};
    }
    if (p->d.error.hasError() || p->d.next >= p->d.end)
        return {};
    return av::Time::fromFrames(p->d.next++, p->d.fps);
}

av::Time
BrawReader::seek(const av::TimeRange& range)
{
    QMutexLocker lock(&p->d.mutex);
    if (!p->d.open || !range.isValid()) {
        p->d.error = core::Error("brawreader", "invalid seek or reader is not open");
        return {};
    }
    const auto start = av::Time::convert(range.start(), p->d.fps).frames();
    const auto end = av::Time::convert(range.end(), p->d.fps).frames();
    if (start < 0 || start >= p->d.range.duration().frames() || end <= start || end > p->d.range.duration().frames()) {
        p->d.error = core::Error("brawreader", "seek range is outside the clip");
        return {};
    }
    p->d.next = start;
    p->d.end = end;
    p->d.image = {};
    p->d.currentMetadata = p->d.clipMetadata;
    p->d.error = {};
    return av::Time::fromFrames(start, p->d.fps);
}

av::Time
BrawReader::start() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.open ? p->d.range.start() : av::Time();
}

av::Time
BrawReader::time() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.open ? av::Time::fromFrames(p->d.next, p->d.fps) : av::Time();
}

av::Fps
BrawReader::fps() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.fps;
}

av::TimeRange
BrawReader::timeRange() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.range;
}

core::AudioBuffer
BrawReader::audio() const
{
    return {};
}

core::ImageBuffer
BrawReader::image() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.image;
}

core::MetaData
BrawReader::metaData() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.currentMetadata;
}

core::Error
BrawReader::error() const
{
    QMutexLocker lock(&p->d.mutex);
    return p->d.error;
}

plugins::PluginHandler
BrawReader::handler()
{
    static const auto handler = plugins::PluginHandler::create<MediaReader>(
        { "BrawReader", "Reads Blackmagic RAW video", "1.0.0" }, [] { return QList<QString> { "braw" }; },
        []() -> core::Plugin* { return new BrawReader; });
    return handler;
}

}  // namespace flipman::sdk::plugins
