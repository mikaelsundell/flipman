// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"
#include <DeckLinkAPI.h>
#include <flipmansdk/av/media.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/environment.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/render/render.h>
#include <flipmansdk/widgets/viewer.h>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSplitter>
#include <QTimer>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>

namespace flipman {

template<typename T>
static void
safeRelease(T*& p)
{
    if (p) {
        p->Release();
        p = nullptr;
    }
}

struct RGB {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

struct YCbCr10 {
    uint16_t y = 64;
    uint16_t cb = 512;
    uint16_t cr = 512;
};

struct YCbCr8 {
    uint8_t y = 16;
    uint8_t cb = 128;
    uint8_t cr = 128;
};

enum class TestPattern { Bars, ColorPatches, GrayRamp, GrayRampMidWhite, GrayRampThreeWhitePatches };
enum class OutputSource { Pattern, Image };

struct PatternOptions {
    TestPattern pattern = TestPattern::GrayRampMidWhite;
    double gamma = 1.0;
    bool applyGamma = false;
    bool legalRange = true;
};

double
clamp01(double v)
{
    return std::max(0.0, std::min(1.0, v));
}

uint8_t
clamp8(int v)
{
    return uint8_t(std::max(0, std::min(255, v)));
}

uint16_t
clamp10(int v)
{
    return uint16_t(std::max(0, std::min(1023, v)));
}

double
encodeGamma(double linear, double gamma)
{
    gamma = std::max(0.01, gamma);
    return std::pow(clamp01(linear), 1.0 / gamma);
}

RGB
makeGray(double linear, const PatternOptions& options)
{
    double v = clamp01(linear);

    if (options.applyGamma)
        v = encodeGamma(v, options.gamma);

    return { v, v, v };
}

YCbCr10
rgbToRec709Ycbcr10(const RGB& c, bool legalRange)
{
    const double r = clamp01(c.r);
    const double g = clamp01(c.g);
    const double b = clamp01(c.b);

    const double y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    const double cb = (b - y) / 1.8556;
    const double cr = (r - y) / 1.5748;

    if (legalRange) {
        return { clamp10(int(std::round(64.0 + 876.0 * y))), clamp10(int(std::round(512.0 + 896.0 * cb))),
                 clamp10(int(std::round(512.0 + 896.0 * cr))) };
    }

    return { clamp10(int(std::round(1023.0 * y))), clamp10(int(std::round(512.0 + 1023.0 * cb))),
             clamp10(int(std::round(512.0 + 1023.0 * cr))) };
}

YCbCr8
rgbToRec709Ycbcr8(const RGB& c, bool legalRange)
{
    const YCbCr10 v = rgbToRec709Ycbcr10(c, legalRange);

    if (legalRange) {
        return { clamp8(int(std::round(double(v.y) / 4.0))), clamp8(int(std::round(double(v.cb) / 4.0))),
                 clamp8(int(std::round(double(v.cr) / 4.0))) };
    }

    return { clamp8(int(std::round(double(v.y) * 255.0 / 1023.0))),
             clamp8(int(std::round(double(v.cb) * 255.0 / 1023.0))),
             clamp8(int(std::round(double(v.cr) * 255.0 / 1023.0))) };
}

RGB
colorPatchRgbAt(int x, int y, int width, int height, const PatternOptions& options)
{
    const int topHeight = height * 3 / 4;

    if (y >= topHeight) {
        constexpr std::array<double, 8> grays = { 0.0, 0.05, 0.10, 0.18, 0.25, 0.50, 0.75, 1.0 };
        const int index = std::clamp((x * int(grays.size())) / std::max(1, width), 0, int(grays.size()) - 1);
        return makeGray(grays[index], options);
    }

    constexpr std::array<RGB, 12> patches = { { { 1.0, 1.0, 1.0 },
                                                { 0.75, 0.75, 0.75 },
                                                { 0.50, 0.50, 0.50 },
                                                { 0.0, 0.0, 0.0 },
                                                { 1.0, 0.0, 0.0 },
                                                { 0.0, 1.0, 0.0 },
                                                { 0.0, 0.0, 1.0 },
                                                { 0.18, 0.18, 0.18 },
                                                { 1.0, 1.0, 0.0 },
                                                { 0.0, 1.0, 1.0 },
                                                { 1.0, 0.0, 1.0 },
                                                { 0.10, 0.10, 0.10 } } };

    const int cols = 4;
    const int rows = 3;

    const int col = std::clamp((x * cols) / std::max(1, width), 0, cols - 1);
    const int row = std::clamp((y * rows) / std::max(1, topHeight), 0, rows - 1);
    const int index = std::clamp(row * cols + col, 0, int(patches.size()) - 1);

    return patches[index];
}

RGB
patternRgbAt(int x, int y, int width, int height, const PatternOptions& options)
{
    if (options.pattern == TestPattern::Bars) {
        constexpr std::array<RGB, 8> bars = { { { 0.75, 0.75, 0.75 },
                                                { 0.75, 0.75, 0.00 },
                                                { 0.00, 0.75, 0.75 },
                                                { 0.00, 0.75, 0.00 },
                                                { 0.75, 0.00, 0.75 },
                                                { 0.75, 0.00, 0.00 },
                                                { 0.00, 0.00, 0.75 },
                                                { 0.00, 0.00, 0.00 } } };

        const int index = std::min<int>(x * int(bars.size()) / std::max(1, width), int(bars.size()) - 1);
        return bars[index];
    }

    if (options.pattern == TestPattern::ColorPatches)
        return colorPatchRgbAt(x, y, width, height, options);

    const double linearRamp = double(x) / double(std::max(1, width - 1));
    RGB rgb = makeGray(linearRamp, options);

    const int patchSize = std::min(width, height) / 8;
    const int cy = height / 2;

    auto insideSquare = [&](int centerX, int centerY) {
        return std::abs(x - centerX) <= patchSize / 2 && std::abs(y - centerY) <= patchSize / 2;
    };

    if (options.pattern == TestPattern::GrayRampMidWhite) {
        if (insideSquare(width / 2, cy))
            rgb = makeGray(1.0, options);
    }

    if (options.pattern == TestPattern::GrayRampThreeWhitePatches) {
        if (insideSquare(width / 4, cy) || insideSquare(width / 2, cy) || insideSquare(width * 3 / 4, cy))
            rgb = makeGray(1.0, options);
    }

    return rgb;
}

YCbCr10
patternYcbcr10At(int x, int y, int width, int height, const PatternOptions& options)
{
    return rgbToRec709Ycbcr10(patternRgbAt(x, y, width, height, options), options.legalRange);
}

YCbCr8
patternYcbcr8At(int x, int y, int width, int height, const PatternOptions& options)
{
    return rgbToRec709Ycbcr8(patternRgbAt(x, y, width, height, options), options.legalRange);
}

void
packV210Word(uint8_t* dst, int wordIndex, uint16_t a, uint16_t b, uint16_t c)
{
    const uint32_t word = (uint32_t(a & 0x3ff) << 0) | (uint32_t(b & 0x3ff) << 10) | (uint32_t(c & 0x3ff) << 20);

    dst[wordIndex * 4 + 0] = uint8_t((word >> 0) & 0xff);
    dst[wordIndex * 4 + 1] = uint8_t((word >> 8) & 0xff);
    dst[wordIndex * 4 + 2] = uint8_t((word >> 16) & 0xff);
    dst[wordIndex * 4 + 3] = uint8_t((word >> 24) & 0xff);
}

void
fillRec709Pattern10Bit(IDeckLinkMutableVideoFrame* frame, int width, int height, int rowBytes,
                       const PatternOptions& options)
{
    void* bytes = nullptr;
    frame->GetBytes(&bytes);

    auto* dst = static_cast<uint8_t*>(bytes);

    for (int y = 0; y < height; ++y) {
        auto* row = dst + y * rowBytes;

        for (int x = 0; x < width; x += 6) {
            const YCbCr10 p0 = patternYcbcr10At(x + 0, y, width, height, options);
            const YCbCr10 p1 = patternYcbcr10At(x + 1, y, width, height, options);
            const YCbCr10 p2 = patternYcbcr10At(x + 2, y, width, height, options);
            const YCbCr10 p3 = patternYcbcr10At(x + 3, y, width, height, options);
            const YCbCr10 p4 = patternYcbcr10At(x + 4, y, width, height, options);
            const YCbCr10 p5 = patternYcbcr10At(x + 5, y, width, height, options);

            const uint16_t cb0 = uint16_t((int(p0.cb) + int(p1.cb)) / 2);
            const uint16_t cr0 = uint16_t((int(p0.cr) + int(p1.cr)) / 2);
            const uint16_t cb2 = uint16_t((int(p2.cb) + int(p3.cb)) / 2);
            const uint16_t cr2 = uint16_t((int(p2.cr) + int(p3.cr)) / 2);
            const uint16_t cb4 = uint16_t((int(p4.cb) + int(p5.cb)) / 2);
            const uint16_t cr4 = uint16_t((int(p4.cr) + int(p5.cr)) / 2);

            const int word = (x / 6) * 4;

            packV210Word(row, word + 0, cb0, p0.y, cr0);
            packV210Word(row, word + 1, p1.y, cb2, p2.y);
            packV210Word(row, word + 2, cr2, p3.y, cb4);
            packV210Word(row, word + 3, p4.y, cr4, p5.y);
        }
    }
}

void
fillRec709Pattern8Bit(IDeckLinkMutableVideoFrame* frame, int width, int height, int rowBytes,
                      const PatternOptions& options)
{
    void* bytes = nullptr;
    frame->GetBytes(&bytes);

    auto* dst = static_cast<uint8_t*>(bytes);

    for (int y = 0; y < height; ++y) {
        auto* row = dst + y * rowBytes;

        for (int x = 0; x < width; x += 2) {
            const YCbCr8 p0 = patternYcbcr8At(x + 0, y, width, height, options);
            const YCbCr8 p1 = patternYcbcr8At(x + 1, y, width, height, options);

            const uint8_t cb = uint8_t((int(p0.cb) + int(p1.cb)) / 2);
            const uint8_t cr = uint8_t((int(p0.cr) + int(p1.cr)) / 2);

            row[x * 2 + 0] = cb;
            row[x * 2 + 1] = p0.y;
            row[x * 2 + 2] = cr;
            row[x * 2 + 3] = p1.y;
        }
    }
}

class DeckLinkRenderOutput : public sdk::render::RenderOutput {
public:
    using FrameCallback = std::function<void(const sdk::core::ImageBuffer&, qint64)>;
    explicit DeckLinkRenderOutput(QObject* parent = nullptr)
        : sdk::render::RenderOutput(parent)
    {}
    void setFrameCallback(FrameCallback callback) { m_callback = std::move(callback); }
    void enqueueFrame(const sdk::core::ImageBuffer& image, qint64 frame) override
    {
        qDebug() << "decklink renderoutput: enqueueFrame"
                 << "frame" << frame << "valid" << image.isValid() << "allocated" << image.isAllocated() << "dataWindow"
                 << image.dataWindow() << "displayWindow" << image.displayWindow() << "format"
                 << int(image.imageFormat().type()) << "channels" << image.channels() << "packing"
                 << int(image.packing()) << "stride" << image.strideSize() << "bytes" << image.byteSize();

        if (m_callback)
            m_callback(image, frame);
    }

private:
    FrameCallback m_callback;
};

class WindowPrivate : public QObject {
public:
    WindowPrivate();
    ~WindowPrivate() override;
    void init();
    void closeDeckLink();
    bool openDeckLink();
    void populateDevices();
    void resendFrame();
    void send();
    void sendPattern();
    void sendImage();
    void setControlsEnabled(bool enabled);
    void setGammaAndSend(double gamma);
    void stopOutput();
    void updateStatus(const QString& text);
    bool createDeckLinkFrame(BMDPixelFormat pixelFormat, int width, int height, int rowBytes);
    bool configureDeckLinkOutput();
    bool copyImageToDeckLinkFrame(const sdk::core::ImageBuffer& image);
    void receiveRenderedImage(const sdk::core::ImageBuffer& image, qint64 frame);
    void updateRenderOutputFormat();
    void updateRenderOutputSpec();
    void updateViewer();
    void loadImage();
    OutputSource currentSource() const;
    PatternOptions patternOptions() const;
    TestPattern currentPattern() const;

public:
    struct DisplayTransform {
        QString label;
        sdk::render::DisplayTransform transform;
    };
    QVector<DisplayTransform> displayTransforms();

public:
    struct Data {
        QString inputFile;
        QString dataPath;
        QPointer<Window> window;
        QPointer<QComboBox> displayCombo;
        QPointer<QComboBox> sourceCombo;
        QPointer<QComboBox> deviceCombo;
        QPointer<QComboBox> formatCombo;
        QPointer<QComboBox> modeCombo;
        QPointer<QComboBox> patternCombo;
        QPointer<QDoubleSpinBox> gammaSpin;
        QPointer<QCheckBox> gammaCheck;
        QPointer<QCheckBox> legalRangeCheck;
        QPointer<QPushButton> gamma22Button;
        QPointer<QPushButton> gamma24Button;
        QPointer<QPushButton> sendButton;
        QPointer<QPushButton> stopButton;
        QPointer<QLabel> statusLabel;
        QPointer<QTimer> outputTimer;
        QPointer<QWidget> controlsWidget;
        QPointer<sdk::widgets::Viewer> viewer;
        QPointer<sdk::render::RenderEngine> renderEngine;
        QPointer<DeckLinkRenderOutput> renderOutput;
        sdk::render::ImageLayer imageLayer;
        sdk::av::Media media;
        IDeckLinkIterator* iterator = nullptr;
        IDeckLink* deckLink = nullptr;
        IDeckLinkOutput* output = nullptr;
        IDeckLinkMutableVideoFrame* frame = nullptr;
        int width = 1920;
        int height = 1080;
        int rowBytes = 5120;
        BMDDisplayMode displayMode = bmdModeHD1080p25;
        BMDDisplayMode activeDisplayMode = bmdModeUnknown;
        BMDPixelFormat pixelFormat = bmdFormat10BitYUV;
        bool outputEnabled = false;
        bool waitingForImageFrame = false;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

WindowPrivate::~WindowPrivate() { closeDeckLink(); }

void
WindowPrivate::updateStatus(const QString& text)
{
    if (d.statusLabel)
        d.statusLabel->setText(text);
}

OutputSource
WindowPrivate::currentSource() const
{
    const int index = d.sourceCombo ? d.sourceCombo->currentIndex() : 0;
    return index == 1 ? OutputSource::Image : OutputSource::Pattern;
}

void
WindowPrivate::setControlsEnabled(bool enabled)
{
    if (d.controlsWidget)
        d.controlsWidget->setEnabled(enabled);

    if (d.sendButton)
        d.sendButton->setEnabled(enabled);

    if (d.stopButton)
        d.stopButton->setEnabled(enabled);

    if (d.gamma22Button)
        d.gamma22Button->setEnabled(enabled);

    if (d.gamma24Button)
        d.gamma24Button->setEnabled(enabled);
}

void
WindowPrivate::populateDevices()
{
    if (!d.deviceCombo)
        return;

    d.deviceCombo->clear();

    IDeckLinkIterator* iterator = CreateDeckLinkIteratorInstance();
    if (!iterator) {
        d.deviceCombo->addItem("Desktop Video not available", -1);
        d.deviceCombo->setEnabled(false);
        setControlsEnabled(false);
        updateStatus("Could not create DeckLink iterator. Is Blackmagic Desktop Video installed?");
        return;
    }

    int index = 0;
    IDeckLink* deckLink = nullptr;

    while (iterator->Next(&deckLink) == S_OK && deckLink) {
        CFStringRef displayName = nullptr;
        QString name = QString("DeckLink Device %1").arg(index + 1);

        if (deckLink->GetDisplayName(&displayName) == S_OK && displayName) {
            name = QString::fromCFString(displayName);
            CFRelease(displayName);
        }

        d.deviceCombo->addItem(name, index);

        safeRelease(deckLink);
        ++index;
    }

    safeRelease(iterator);

    const bool hasDevices = index > 0;

    if (!hasDevices) {
        d.deviceCombo->addItem("No DeckLink / UltraStudio device found", -1);
        d.deviceCombo->setEnabled(false);
        setControlsEnabled(false);
        updateStatus("No DeckLink / UltraStudio device found.");
        return;
    }

    d.deviceCombo->setEnabled(true);
    d.deviceCombo->setCurrentIndex(0);
    setControlsEnabled(true);
    updateStatus(QString("Found %1 DeckLink device%2.").arg(index).arg(index == 1 ? "" : "s"));
}

TestPattern
WindowPrivate::currentPattern() const
{
    const int patternIndex = d.patternCombo ? d.patternCombo->currentIndex() : 0;

    switch (patternIndex) {
    case 0: return TestPattern::Bars;
    case 1: return TestPattern::ColorPatches;
    case 2: return TestPattern::GrayRamp;
    case 3: return TestPattern::GrayRampMidWhite;
    case 4: return TestPattern::GrayRampThreeWhitePatches;
    default: return TestPattern::GrayRampMidWhite;
    }
}

PatternOptions
WindowPrivate::patternOptions() const
{
    PatternOptions options;
    options.pattern = currentPattern();
    options.gamma = d.gammaSpin ? d.gammaSpin->value() : 2.4;
    options.applyGamma = d.gammaCheck ? d.gammaCheck->isChecked() : false;
    options.legalRange = d.legalRangeCheck ? d.legalRangeCheck->isChecked() : true;
    return options;
}

void
WindowPrivate::setGammaAndSend(double gamma)
{
    if (d.gammaSpin) {
        QSignalBlocker blocker(d.gammaSpin);
        d.gammaSpin->setValue(gamma);
    }

    if (currentSource() == OutputSource::Pattern)
        sendPattern();
}

bool
WindowPrivate::openDeckLink()
{
    if (d.output)
        return true;

    const int selectedDevice = d.deviceCombo ? d.deviceCombo->currentData().toInt() : -1;
    if (selectedDevice < 0) {
        updateStatus("No DeckLink / UltraStudio device selected.");
        return false;
    }

    d.iterator = CreateDeckLinkIteratorInstance();
    if (!d.iterator) {
        updateStatus("Could not create DeckLink iterator. Is Blackmagic Desktop Video installed?");
        return false;
    }

    int index = 0;
    IDeckLink* deckLink = nullptr;

    while (d.iterator->Next(&deckLink) == S_OK && deckLink) {
        if (index == selectedDevice) {
            d.deckLink = deckLink;
            deckLink = nullptr;
            break;
        }

        safeRelease(deckLink);
        ++index;
    }

    if (!d.deckLink) {
        updateStatus("Selected DeckLink / UltraStudio device could not be opened.");
        closeDeckLink();
        return false;
    }

    if (d.deckLink->QueryInterface(IID_IDeckLinkOutput, reinterpret_cast<void**>(&d.output)) != S_OK || !d.output) {
        updateStatus("Selected DeckLink device does not support output.");
        closeDeckLink();
        return false;
    }

    updateStatus("DeckLink output device opened.");
    return true;
}

void
WindowPrivate::closeDeckLink()
{
    stopOutput();

    safeRelease(d.frame);
    safeRelease(d.output);
    safeRelease(d.deckLink);
    safeRelease(d.iterator);
}

void
WindowPrivate::stopOutput()
{
    if (d.outputTimer)
        d.outputTimer->stop();

    safeRelease(d.frame);

    if (d.output && d.outputEnabled) {
        d.output->DisableVideoOutput();
        d.outputEnabled = false;
        d.activeDisplayMode = bmdModeUnknown;
        updateStatus("Output stopped.");
    }

    d.waitingForImageFrame = false;
}

void
WindowPrivate::resendFrame()
{
    if (!d.output || !d.outputEnabled || !d.frame)
        return;

    d.output->DisplayVideoFrameSync(d.frame);
}

bool
WindowPrivate::configureDeckLinkOutput()
{
    if (!openDeckLink())
        return false;

    if (d.modeCombo) {
        const QVariant modeData = d.modeCombo->currentData();
        if (modeData.isValid())
            d.displayMode = BMDDisplayMode(modeData.toInt());
    }

    if (d.formatCombo) {
        const QVariant formatData = d.formatCombo->currentData();
        if (formatData.isValid())
            d.pixelFormat = BMDPixelFormat(formatData.toInt());
    }

    d.width = 1920;
    d.height = 1080;

    const bool needEnable = !d.outputEnabled || d.activeDisplayMode != d.displayMode;

    if (needEnable) {
        if (d.outputTimer)
            d.outputTimer->stop();

        safeRelease(d.frame);

        if (d.outputEnabled) {
            d.output->DisableVideoOutput();
            d.outputEnabled = false;
            d.activeDisplayMode = bmdModeUnknown;
        }

        if (d.output->EnableVideoOutput(d.displayMode, bmdVideoOutputFlagDefault) != S_OK) {
            updateStatus("Failed to enable selected DeckLink video output mode.");
            return false;
        }

        d.outputEnabled = true;
        d.activeDisplayMode = d.displayMode;
    }

    return true;
}

bool
WindowPrivate::createDeckLinkFrame(BMDPixelFormat pixelFormat, int width, int height, int rowBytes)
{
    if (!d.output)
        return false;

    safeRelease(d.frame);

    if (d.output->CreateVideoFrame(width, height, rowBytes, pixelFormat, bmdFrameFlagDefault, &d.frame) != S_OK
        || !d.frame) {
        updateStatus("Failed to create DeckLink video frame.");
        stopOutput();
        return false;
    }

    d.width = width;
    d.height = height;
    d.rowBytes = rowBytes;
    d.pixelFormat = pixelFormat;

    return true;
}

void
WindowPrivate::send()
{
    if (currentSource() == OutputSource::Image)
        sendImage();
    else
        sendPattern();
}

void
WindowPrivate::sendPattern()
{
    if (!configureDeckLinkOutput())
        return;

    if (d.pixelFormat == bmdFormat10BitYUV)
        d.rowBytes = ((d.width + 47) / 48) * 128;
    else
        d.rowBytes = d.width * 2;

    if (!createDeckLinkFrame(d.pixelFormat, d.width, d.height, d.rowBytes))
        return;

    const PatternOptions options = patternOptions();

    if (d.pixelFormat == bmdFormat10BitYUV)
        fillRec709Pattern10Bit(d.frame, d.width, d.height, d.rowBytes, options);
    else
        fillRec709Pattern8Bit(d.frame, d.width, d.height, d.rowBytes, options);

    resendFrame();

    if (d.outputTimer && !d.outputTimer->isActive())
        d.outputTimer->start(40);

    updateStatus(QString("Sending Rec.709 %1 pattern, gamma %2 %3, %4.")
                     .arg(d.pixelFormat == bmdFormat10BitYUV ? "10-bit YUV" : "8-bit UYVY")
                     .arg(QString::number(options.gamma, 'f', 2))
                     .arg(options.applyGamma ? "enabled" : "disabled")
                     .arg(options.legalRange ? "legal range" : "full range"));
}

void
WindowPrivate::sendImage()
{
    if (!configureDeckLinkOutput())
        return;

    updateRenderOutputFormat();
    updateRenderOutputSpec();

    d.waitingForImageFrame = true;

    if (d.renderEngine)
        d.renderEngine->setImageLayers({ d.imageLayer });

    if (d.viewer)
        d.viewer->update();

    updateStatus("Rendering image and waiting for YUV readback...");
}

bool
WindowPrivate::copyImageToDeckLinkFrame(const sdk::core::ImageBuffer& image)
{
    if (!image.isValid() || !image.isAllocated())
        return false;

    const QRect displayWindow = image.displayWindow();
    const int width = displayWindow.width();
    const int height = displayWindow.height();
    const int imageRowBytes = int(image.strideSize());

    if (width <= 0 || height <= 0 || imageRowBytes <= 0)
        return false;

    BMDPixelFormat pixelFormat = bmdFormat8BitYUV;
    int deckLinkRowBytes = imageRowBytes;

    if (d.pixelFormat == bmdFormat10BitYUV) {
        pixelFormat = bmdFormat10BitYUV;
        deckLinkRowBytes = ((width + 47) / 48) * 128;
    }
    else {
        pixelFormat = bmdFormat8BitYUV;
        deckLinkRowBytes = width * 2;
    }

    if (imageRowBytes != deckLinkRowBytes) {
        qWarning() << "DeckLink row bytes mismatch"
                   << "image" << imageRowBytes << "decklink" << deckLinkRowBytes;
        return false;
    }

    if (!createDeckLinkFrame(pixelFormat, width, height, deckLinkRowBytes))
        return false;

    void* frameBytes = nullptr;
    d.frame->GetBytes(&frameBytes);

    if (!frameBytes)
        return false;

    auto* dst = static_cast<uint8_t*>(frameBytes);
    const auto* src = static_cast<const uint8_t*>(image.data());

    for (int y = 0; y < height; ++y)
        std::memcpy(dst + size_t(y) * deckLinkRowBytes, src + size_t(y) * imageRowBytes, size_t(deckLinkRowBytes));

    return true;
}

void
WindowPrivate::receiveRenderedImage(const sdk::core::ImageBuffer& image, qint64 frame)
{
    if (!d.waitingForImageFrame)
        return;

    d.waitingForImageFrame = false;

    if (!copyImageToDeckLinkFrame(image)) {
        updateStatus("Failed to copy rendered image to DeckLink frame.");
        return;
    }

    resendFrame();

    if (d.outputTimer && !d.outputTimer->isActive())
        d.outputTimer->start(40);

    updateStatus(QString("Sending rendered image frame %1 as %2.")
                     .arg(frame)
                     .arg(d.pixelFormat == bmdFormat10BitYUV ? "v210 / 10-bit YUV" : "2vuy / UYVY8"));
}

void
WindowPrivate::updateRenderOutputFormat()
{
    if (!d.renderOutput)
        return;

    if (d.pixelFormat == bmdFormat10BitYUV)
        d.renderOutput->setFormat(sdk::render::RenderOutput::Format::V210);
    else
        d.renderOutput->setFormat(sdk::render::RenderOutput::Format::UYVY8);
}

void
WindowPrivate::updateRenderOutputSpec()
{
    if (!d.renderOutput || !d.renderEngine)
        return;

    sdk::render::RenderSpec spec;
    spec.setSize(QSize(d.width, d.height));

    QMatrix4x4 view;
    view.setToIdentity();
    spec.setView(view);

    d.renderOutput->setRenderSpec(spec);
    d.renderEngine->setResolution(QSize(d.width, d.height));
}

void
WindowPrivate::updateViewer()
{
    if (!d.viewer || !d.renderEngine)
        return;

    d.renderEngine->setImageLayers({ d.imageLayer });
    d.viewer->update();
}

void
WindowPrivate::loadImage()
{
    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1)
        d.inputFile = args.at(1);

    d.dataPath = sdk::core::Environment::resourcePath("../../../data");

    sdk::core::File file(d.inputFile);
    if (!file.exists()) {
        const QString filename = "iphone17pro rec709 gamma2.4 ProRes 4444.mov";
        file = sdk::core::File(QString("%1/quicktime/%2").arg(d.dataPath).arg(filename));
    }

    if (!file.exists()) {
        qWarning() << "testdecklink: image/media file does not exist:" << d.inputFile;
        return;
    }

    if (!d.media.open(file) || !d.media.waitForOpened() || !d.media.isValid()) {
        qWarning() << "testdecklink: could not open media:" << file;
        return;
    }

    d.media.read();

    const sdk::core::ImageBuffer image = d.media.image();
    if (!image.isValid()) {
        qWarning() << "testdecklink: invalid media image";
        return;
    }

    d.imageLayer.setImage(image);
}

void
WindowPrivate::init()
{
    loadImage();

    QWidget* centralWidget = new QWidget(d.window);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget* toolbar = new QWidget(centralWidget);
    toolbar->setFixedHeight(40);
    toolbar->setProperty("role", "toolbar");

    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(12, 0, 12, 0);
    toolbarLayout->setSpacing(8);
    toolbarLayout->setAlignment(Qt::AlignVCenter);

    QLabel* titleLabel = new QLabel("DeckLink Rec.709 Test Generator", toolbar);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    toolbarLayout->addWidget(titleLabel);
    toolbarLayout->addStretch(1);

    QWidget* contentWidget = new QWidget(centralWidget);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(12);

    QLabel* infoLabel = new QLabel(
        "Generates Rec.709 patterns or renders an image and sends YUV frames to the UltraStudio / DeckLink output.",
        contentWidget);
    infoLabel->setWordWrap(true);

    QGroupBox* imageBox = new QGroupBox("Image", contentWidget);
    QVBoxLayout* imageLayout = new QVBoxLayout(imageBox);
    imageLayout->setContentsMargins(10, 10, 10, 10);
    imageLayout->setSpacing(8);

    d.renderOutput = new DeckLinkRenderOutput(d.window);
    d.renderOutput->setEnabled(true);
    d.renderOutput->setFormat(sdk::render::RenderOutput::Format::UYVY8);
    d.renderOutput->setFrameCallback(
        [this](const sdk::core::ImageBuffer& image, qint64 frame) { receiveRenderedImage(image, frame); });

    sdk::render::RenderSpec outputSpec;
    outputSpec.setSize(QSize(d.width, d.height));
    d.renderOutput->setRenderSpec(outputSpec);

    d.renderEngine = new sdk::render::RenderEngine(d.window);
    d.renderEngine->setResolution(QSize(d.width, d.height));
    d.renderEngine->setRenderOutputs({ d.renderOutput });

    d.viewer = new sdk::widgets::Viewer(imageBox);
    d.viewer->setRenderEngine(d.renderEngine.data());
    d.viewer->setDisplayTransform({ sdk::render::ColorSpace::Rec709, sdk::render::TransferFunction::Gamma24 });
    d.viewer->setMinimumHeight(180);
    d.viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    imageLayout->addWidget(d.viewer);

    QGroupBox* outputBox = new QGroupBox("Output", contentWidget);
    QVBoxLayout* outputLayout = new QVBoxLayout(outputBox);
    outputLayout->setSpacing(10);

    d.deviceCombo = new QComboBox(outputBox);

    d.controlsWidget = new QWidget(outputBox);
    QVBoxLayout* controlsLayout = new QVBoxLayout(d.controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(10);

    d.displayCombo = new QComboBox(d.controlsWidget);
    d.displayCombo->setMinimumWidth(220);

    const QVector<DisplayTransform> transforms = displayTransforms();

    for (int i = 0; i < transforms.size(); ++i)
        d.displayCombo->addItem(transforms[i].label, i);

    int displayIndex = 0;
    const sdk::render::DisplayTransform defaultDisplayTransform { sdk::render::ColorSpace::Rec709,
                                                                  sdk::render::TransferFunction::Gamma24 };

    for (int i = 0; i < transforms.size(); ++i) {
        if (transforms[i].transform.colorSpace == defaultDisplayTransform.colorSpace
            && transforms[i].transform.transferFunction == defaultDisplayTransform.transferFunction) {
            displayIndex = i;
            break;
        }
    }

    d.displayCombo->setCurrentIndex(displayIndex);
    d.viewer->setDisplayTransform(defaultDisplayTransform);

    d.sourceCombo = new QComboBox(d.controlsWidget);
    d.sourceCombo->addItem("Pattern", int(OutputSource::Pattern));
    d.sourceCombo->addItem("Image", int(OutputSource::Image));

    d.modeCombo = new QComboBox(d.controlsWidget);
    d.modeCombo->addItem("1080p25", QVariant::fromValue(int(bmdModeHD1080p25)));
    d.modeCombo->addItem("1080p24", QVariant::fromValue(int(bmdModeHD1080p24)));
    d.modeCombo->addItem("1080p2398", QVariant::fromValue(int(bmdModeHD1080p2398)));
    d.modeCombo->addItem("1080i50", QVariant::fromValue(int(bmdModeHD1080i50)));

    d.formatCombo = new QComboBox(d.controlsWidget);
    d.formatCombo->addItem("10-bit YUV", QVariant::fromValue(int(bmdFormat10BitYUV)));
    d.formatCombo->addItem("8-bit UYVY", QVariant::fromValue(int(bmdFormat8BitYUV)));
    d.formatCombo->setCurrentIndex(1);

    d.patternCombo = new QComboBox(d.controlsWidget);
    d.patternCombo->addItem("Color bars 75%");
    d.patternCombo->addItem("Rec.709 patches + gray axis");
    d.patternCombo->addItem("Gray ramp");
    d.patternCombo->addItem("Gray ramp + center white");
    d.patternCombo->addItem("Gray ramp + 3 white patches");
    d.patternCombo->setCurrentIndex(3);

    d.gammaSpin = new QDoubleSpinBox(d.controlsWidget);
    d.gammaSpin->setDecimals(2);
    d.gammaSpin->setRange(1.00, 3.00);
    d.gammaSpin->setSingleStep(0.10);
    d.gammaSpin->setValue(2.40);
    d.gammaSpin->setFixedWidth(90);
    d.gammaSpin->setAlignment(Qt::AlignCenter);

    d.gammaCheck = new QCheckBox("Apply gamma encoding", d.controlsWidget);
    d.gammaCheck->setChecked(false);

    d.legalRangeCheck = new QCheckBox("Legal video range", d.controlsWidget);
    d.legalRangeCheck->setChecked(true);

    auto addRow = [](QVBoxLayout* parentLayout, const QString& label, QWidget* widget, bool expanding) {
        QWidget* row = new QWidget(parentLayout->parentWidget());
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(10);

        QLabel* title = new QLabel(label, row);
        title->setFixedWidth(100);
        title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        layout->addWidget(title);

        if (expanding) {
            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            layout->addWidget(widget, 1);
        }
        else {
            layout->addWidget(widget, 0, Qt::AlignLeft | Qt::AlignVCenter);
            layout->addStretch(1);
        }

        parentLayout->addWidget(row);
    };

    addRow(controlsLayout, "Display", d.displayCombo, true);
    addRow(controlsLayout, "Source", d.sourceCombo, true);
    addRow(controlsLayout, "Device", d.deviceCombo, true);
    addRow(controlsLayout, "Mode", d.modeCombo, true);
    addRow(controlsLayout, "Format", d.formatCombo, true);
    addRow(controlsLayout, "Pattern", d.patternCombo, true);
    addRow(controlsLayout, "Gamma", d.gammaSpin, false);
    controlsLayout->addWidget(d.gammaCheck);
    controlsLayout->addWidget(d.legalRangeCheck);

    outputLayout->addWidget(d.controlsWidget);

    QWidget* buttonsRow = new QWidget(contentWidget);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsRow);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(8);

    d.sendButton = new QPushButton("Send", buttonsRow);
    d.stopButton = new QPushButton("Stop Output", buttonsRow);
    d.gamma22Button = new QPushButton("2.2", buttonsRow);
    d.gamma24Button = new QPushButton("2.4", buttonsRow);

    buttonsLayout->addWidget(d.sendButton);
    buttonsLayout->addWidget(d.stopButton);
    buttonsLayout->addSpacing(16);
    buttonsLayout->addWidget(d.gamma22Button);
    buttonsLayout->addWidget(d.gamma24Button);
    buttonsLayout->addStretch();

    QFrame* statusFrame = new QFrame(contentWidget);
    statusFrame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout* statusLayout = new QVBoxLayout(statusFrame);
    statusLayout->setContentsMargins(10, 8, 10, 8);

    d.statusLabel = new QLabel("Ready.", statusFrame);
    d.statusLabel->setWordWrap(true);
    statusLayout->addWidget(d.statusLabel);

    d.outputTimer = new QTimer(this);
    d.outputTimer->setTimerType(Qt::PreciseTimer);

    contentLayout->addWidget(infoLabel);
    contentLayout->addWidget(imageBox, 1);
    contentLayout->addWidget(outputBox, 0);
    contentLayout->addWidget(buttonsRow, 0);
    contentLayout->addWidget(statusFrame, 0);

    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(contentWidget, 1);

    QObject::connect(d.outputTimer, &QTimer::timeout, this, [this]() { resendFrame(); });

    QObject::connect(d.deviceCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        closeDeckLink();
        setControlsEnabled(d.deviceCombo && d.deviceCombo->currentData().toInt() >= 0);
    });

    QObject::connect(d.sendButton, &QPushButton::clicked, this, [this]() { send(); });
    QObject::connect(d.stopButton, &QPushButton::clicked, this, [this]() { stopOutput(); });
    QObject::connect(d.gamma22Button, &QPushButton::clicked, this, [this]() { setGammaAndSend(2.20); });
    QObject::connect(d.gamma24Button, &QPushButton::clicked, this, [this]() { setGammaAndSend(2.40); });

    QObject::connect(d.gammaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        if (currentSource() == OutputSource::Pattern)
            sendPattern();
    });

    QObject::connect(d.gammaCheck, &QCheckBox::toggled, this, [this](bool) {
        if (currentSource() == OutputSource::Pattern)
            sendPattern();
    });

    QObject::connect(d.patternCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (currentSource() == OutputSource::Pattern)
            sendPattern();
    });

    QObject::connect(d.legalRangeCheck, &QCheckBox::toggled, this, [this](bool) {
        if (currentSource() == OutputSource::Pattern)
            sendPattern();
    });

    QObject::connect(d.modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (currentSource() == OutputSource::Pattern)
            sendPattern();
        else
            updateRenderOutputSpec();
    });

    QObject::connect(d.formatCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        updateRenderOutputFormat();

        if (currentSource() == OutputSource::Pattern)
            sendPattern();
    });

    QObject::connect(d.displayCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
                     [this, transforms](int index) {
                         if (index < 0)
                             return;

                         const int transformIndex = d.displayCombo->itemData(index).toInt();
                         if (transformIndex < 0 || transformIndex >= transforms.size())
                             return;

                         const DisplayTransform& displayTransform = transforms[transformIndex];

                         if (d.viewer) {
                             d.viewer->setDisplayTransform({ displayTransform.transform.colorSpace,
                                                             displayTransform.transform.transferFunction });
                             d.viewer->update();
                         }

                         if (currentSource() == OutputSource::Image)
                             sendImage();
                     });

    QObject::connect(d.sourceCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        const bool pattern = currentSource() == OutputSource::Pattern;

        if (d.patternCombo)
            d.patternCombo->setEnabled(pattern);
        if (d.gammaSpin)
            d.gammaSpin->setEnabled(pattern);
        if (d.gammaCheck)
            d.gammaCheck->setEnabled(pattern);
        if (d.legalRangeCheck)
            d.legalRangeCheck->setEnabled(pattern);
        if (d.sendButton)
            d.sendButton->setText(pattern ? "Send Pattern" : "Send Image");

        updateStatus(pattern ? "Pattern output selected." : "Image output selected.");
    });

    setControlsEnabled(false);
    populateDevices();

    updateViewer();

    if (d.sourceCombo)
        d.sourceCombo->setCurrentIndex(0);

    d.window->setWindowTitle("testdecklink");
    d.window->setCentralWidget(centralWidget);
    d.window->resize(720, 720);
}

QVector<WindowPrivate::DisplayTransform>
WindowPrivate::displayTransforms()
{
    using ColorSpace = sdk::render::ColorSpace;
    using TransferFunction = sdk::render::TransferFunction;
    return {
        { "sRGB", { ColorSpace::Rec709, TransferFunction::SRGB } },
        { "Rec.709 Gamma 2.2", { ColorSpace::Rec709, TransferFunction::Gamma22 } },
        { "Rec.709 Gamma 2.4", { ColorSpace::Rec709, TransferFunction::Gamma24 } },
        { "Rec.709 Gamma 2.6", { ColorSpace::Rec709, TransferFunction::Gamma26 } },
        { "Display P3 sRGB", { ColorSpace::DisplayP3, TransferFunction::SRGB } },
        { "Display P3 Gamma 2.2", { ColorSpace::DisplayP3, TransferFunction::Gamma22 } },
        { "Display P3 Gamma 2.4", { ColorSpace::DisplayP3, TransferFunction::Gamma24 } },
        { "DCI-P3 Gamma 2.6", { ColorSpace::DCIP3, TransferFunction::Gamma26 } },
        { "Rec.2020 Gamma 2.4", { ColorSpace::Rec2020, TransferFunction::Gamma24 } },
    };
}

Window::Window(QWidget* parent)
    : sdk::widgets::Window(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}

}  // namespace flipman
