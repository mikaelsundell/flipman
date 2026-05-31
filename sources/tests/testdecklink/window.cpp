// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"

#include <DeckLinkAPI.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace flipman {

template <typename T>
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

enum class TestPattern {
    Bars,
    ColorPatches,
    GrayRamp,
    GrayRampMidWhite,
    GrayRampThreeWhitePatches,
    Pluge
};

struct PatternOptions {
    TestPattern pattern = TestPattern::GrayRampMidWhite;
    double gamma = 1.0;
    bool applyGamma = false;
    bool legalRange = true;
};

static double
clamp01(double v)
{
    return std::max(0.0, std::min(1.0, v));
}

static uint16_t
clamp10(int v)
{
    return uint16_t(std::max(0, std::min(1023, v)));
}

static double
encodeGamma(double linear, double gamma)
{
    gamma = std::max(0.01, gamma);
    return std::pow(clamp01(linear), 1.0 / gamma);
}

static RGB
makeGray(double linear, const PatternOptions& options)
{
    double v = clamp01(linear);

    if (options.applyGamma)
        v = encodeGamma(v, options.gamma);

    return { v, v, v };
}

static YCbCr10
makeYcbcrCode(int y, int cb = 512, int cr = 512)
{
    return {
        clamp10(y),
        clamp10(cb),
        clamp10(cr)
    };
}

static YCbCr10
rgbToRec709Ycbcr10(const RGB& c, bool legalRange)
{
    const double r = clamp01(c.r);
    const double g = clamp01(c.g);
    const double b = clamp01(c.b);

    const double y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    const double cb = (b - y) / 1.8556;
    const double cr = (r - y) / 1.5748;

    if (legalRange) {
        return {
            clamp10(int(std::round(64.0 + 876.0 * y))),
            clamp10(int(std::round(512.0 + 896.0 * cb))),
            clamp10(int(std::round(512.0 + 896.0 * cr)))
        };
    }

    return {
        clamp10(int(std::round(1023.0 * y))),
        clamp10(int(std::round(512.0 + 1023.0 * cb))),
        clamp10(int(std::round(512.0 + 1023.0 * cr)))
    };
}

static RGB
colorPatchRgbAt(int x, int y, int width, int height)
{
    constexpr std::array<RGB, 8> patches = {{
        { 1.0, 1.0, 1.0 }, // white
        { 1.0, 1.0, 0.0 }, // yellow
        { 0.0, 1.0, 1.0 }, // cyan
        { 0.0, 1.0, 0.0 }, // green
        { 1.0, 0.0, 1.0 }, // magenta
        { 1.0, 0.0, 0.0 }, // red
        { 0.0, 0.0, 1.0 }, // blue
        { 0.0, 0.0, 0.0 }  // black
    }};

    const int cols = 4;
    const int rows = 2;

    const int col = std::clamp((x * cols) / std::max(1, width), 0, cols - 1);
    const int row = std::clamp((y * rows) / std::max(1, height), 0, rows - 1);
    const int index = std::clamp(row * cols + col, 0, int(patches.size()) - 1);

    return patches[index];
}

static RGB
patternRgbAt(int x, int y, int width, int height, const PatternOptions& options)
{
    if (options.pattern == TestPattern::Bars) {
        constexpr std::array<RGB, 8> bars = {{
            { 0.75, 0.75, 0.75 },
            { 0.75, 0.75, 0.00 },
            { 0.00, 0.75, 0.75 },
            { 0.00, 0.75, 0.00 },
            { 0.75, 0.00, 0.75 },
            { 0.75, 0.00, 0.00 },
            { 0.00, 0.00, 0.75 },
            { 0.00, 0.00, 0.00 }
        }};

        const int index = std::min<int>(
            x * int(bars.size()) / std::max(1, width),
            int(bars.size()) - 1);

        return bars[index];
    }

    if (options.pattern == TestPattern::ColorPatches)
        return colorPatchRgbAt(x, y, width, height);

    const double linearRamp = double(x) / double(std::max(1, width - 1));
    RGB rgb = makeGray(linearRamp, options);

    const int patchSize = std::min(width, height) / 8;
    const int cy = height / 2;

    auto insideSquare = [&](int centerX, int centerY) {
        return std::abs(x - centerX) <= patchSize / 2 &&
               std::abs(y - centerY) <= patchSize / 2;
    };

    if (options.pattern == TestPattern::GrayRampMidWhite) {
        if (insideSquare(width / 2, cy))
            rgb = makeGray(1.0, options);
    }

    if (options.pattern == TestPattern::GrayRampThreeWhitePatches) {
        if (insideSquare(width / 4, cy) ||
            insideSquare(width / 2, cy) ||
            insideSquare(width * 3 / 4, cy)) {
            rgb = makeGray(1.0, options);
        }
    }

    return rgb;
}

static YCbCr10
plugeYcbcrAt(int x, int y, int width, int height, const PatternOptions& options)
{
    const int yBlack = options.legalRange ? 64 : 0;
    const int yWhite = options.legalRange ? 940 : 1023;

    const int yBelowBlack = options.legalRange ? 48 : 0;
    const int yNearBlack1 = options.legalRange ? 80 : 20;
    const int yNearBlack2 = options.legalRange ? 96 : 40;
    const int yDebug1 = options.legalRange ? 128 : 80;
    const int yDebug2 = options.legalRange ? 192 : 140;

    int code = yBlack;

    const int rampTop = height / 8;
    const int rampBottom = height / 3;

    if (y >= rampTop && y < rampBottom) {
        const double t = double(x) / double(std::max(1, width - 1));
        const int rampWhite = options.legalRange ? 256 : 192;
        code = int(std::round(yBlack + double(rampWhite - yBlack) * t));
    }

    const int whiteSize = std::min(width, height) / 6;
    const int whiteX = width - whiteSize - width / 16;
    const int whiteY = height / 10;

    if (x >= whiteX && x < whiteX + whiteSize &&
        y >= whiteY && y < whiteY + whiteSize) {
        code = yWhite;
    }

    const int barTop = height * 2 / 3;
    const int barBottom = height - height / 12;
    const int barWidth = width / 14;
    const int barHeight = barBottom - barTop;
    const int cx = width / 2;

    if (y >= barTop && y < barTop + barHeight) {
        if (x >= cx - barWidth * 3 && x < cx - barWidth * 2)
            code = yBelowBlack;
        else if (x >= cx - barWidth * 2 && x < cx - barWidth)
            code = yBlack;
        else if (x >= cx - barWidth && x < cx)
            code = yNearBlack1;
        else if (x >= cx && x < cx + barWidth)
            code = yNearBlack2;
        else if (x >= cx + barWidth && x < cx + barWidth * 2)
            code = yDebug1;
        else if (x >= cx + barWidth * 2 && x < cx + barWidth * 3)
            code = yDebug2;
    }

    return makeYcbcrCode(code);
}

static YCbCr10
patternYcbcrAt(int x, int y, int width, int height, const PatternOptions& options)
{
    if (options.pattern == TestPattern::Pluge)
        return plugeYcbcrAt(x, y, width, height, options);

    return rgbToRec709Ycbcr10(patternRgbAt(x, y, width, height, options), options.legalRange);
}

static void
packV210Word(uint8_t* dst, int wordIndex, uint16_t a, uint16_t b, uint16_t c)
{
    const uint32_t word =
        (uint32_t(a & 0x3ff) << 0) |
        (uint32_t(b & 0x3ff) << 10) |
        (uint32_t(c & 0x3ff) << 20);

    dst[wordIndex * 4 + 0] = uint8_t((word >> 0) & 0xff);
    dst[wordIndex * 4 + 1] = uint8_t((word >> 8) & 0xff);
    dst[wordIndex * 4 + 2] = uint8_t((word >> 16) & 0xff);
    dst[wordIndex * 4 + 3] = uint8_t((word >> 24) & 0xff);
}

static void
fillRec709Pattern10Bit(IDeckLinkMutableVideoFrame* frame, int width, int height, int rowBytes, const PatternOptions& options)
{
    void* bytes = nullptr;
    frame->GetBytes(&bytes);

    auto* dst = static_cast<uint8_t*>(bytes);

    for (int y = 0; y < height; ++y) {
        auto* row = dst + y * rowBytes;

        for (int x = 0; x < width; x += 6) {
            const YCbCr10 p0 = patternYcbcrAt(x + 0, y, width, height, options);
            const YCbCr10 p1 = patternYcbcrAt(x + 1, y, width, height, options);
            const YCbCr10 p2 = patternYcbcrAt(x + 2, y, width, height, options);
            const YCbCr10 p3 = patternYcbcrAt(x + 3, y, width, height, options);
            const YCbCr10 p4 = patternYcbcrAt(x + 4, y, width, height, options);
            const YCbCr10 p5 = patternYcbcrAt(x + 5, y, width, height, options);

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

class WindowPrivate : public QObject {
public:
    WindowPrivate();
    ~WindowPrivate() override;

    void init();
    bool openDeckLink();
    void closeDeckLink();
    void sendPattern();
    void resendFrame();
    void stopOutput();
    void updateStatus(const QString& text);

    PatternOptions patternOptions() const;
    TestPattern currentPattern() const;
    void setGammaAndSend(double gamma);

    struct Data {
        QPointer<Window> window;
        QPointer<QComboBox> patternCombo;
        QPointer<QComboBox> modeCombo;
        QPointer<QDoubleSpinBox> gammaSpin;
        QPointer<QCheckBox> gammaCheck;
        QPointer<QCheckBox> legalRangeCheck;
        QPointer<QLabel> statusLabel;
        QPointer<QTimer> outputTimer;

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
    };

    Data d;
};

WindowPrivate::WindowPrivate() {}

WindowPrivate::~WindowPrivate()
{
    closeDeckLink();
}

void
WindowPrivate::updateStatus(const QString& text)
{
    if (d.statusLabel)
        d.statusLabel->setText(text);
}

TestPattern
WindowPrivate::currentPattern() const
{
    const int patternIndex = d.patternCombo ? d.patternCombo->currentIndex() : 0;

    switch (patternIndex) {
    case 0:
        return TestPattern::Bars;
    case 1:
        return TestPattern::ColorPatches;
    case 2:
        return TestPattern::GrayRamp;
    case 3:
        return TestPattern::GrayRampMidWhite;
    case 4:
        return TestPattern::GrayRampThreeWhitePatches;
    case 5:
        return TestPattern::Pluge;
    default:
        return TestPattern::GrayRampMidWhite;
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

    sendPattern();
}

bool
WindowPrivate::openDeckLink()
{
    if (d.output)
        return true;

    d.iterator = CreateDeckLinkIteratorInstance();
    if (!d.iterator) {
        updateStatus("Could not create DeckLink iterator. Is Blackmagic Desktop Video installed?");
        return false;
    }

    if (d.iterator->Next(&d.deckLink) != S_OK || !d.deckLink) {
        updateStatus("No DeckLink / UltraStudio device found.");
        closeDeckLink();
        return false;
    }

    if (d.deckLink->QueryInterface(IID_IDeckLinkOutput, reinterpret_cast<void**>(&d.output)) != S_OK || !d.output) {
        updateStatus("DeckLink device does not support output.");
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
}

void
WindowPrivate::resendFrame()
{
    if (!d.output || !d.outputEnabled || !d.frame)
        return;

    d.output->DisplayVideoFrameSync(d.frame);
}

void
WindowPrivate::sendPattern()
{
    if (!openDeckLink())
        return;

    if (d.modeCombo) {
        const QVariant modeData = d.modeCombo->currentData();
        if (modeData.isValid())
            d.displayMode = BMDDisplayMode(modeData.toInt());
    }

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
            return;
        }

        d.outputEnabled = true;
        d.activeDisplayMode = d.displayMode;
    }

    safeRelease(d.frame);

    d.pixelFormat = bmdFormat10BitYUV;
    d.rowBytes = ((d.width + 47) / 48) * 128;

    if (d.output->CreateVideoFrame(
            d.width,
            d.height,
            d.rowBytes,
            d.pixelFormat,
            bmdFrameFlagDefault,
            &d.frame) != S_OK || !d.frame) {
        updateStatus("Failed to create DeckLink video frame.");
        stopOutput();
        return;
    }

    const PatternOptions options = patternOptions();

    fillRec709Pattern10Bit(d.frame, d.width, d.height, d.rowBytes, options);
    resendFrame();

    if (d.outputTimer && !d.outputTimer->isActive())
        d.outputTimer->start(40);

    updateStatus(QString("Sending Rec.709 10-bit YUV pattern, gamma %1 %2, %3.")
                     .arg(QString::number(options.gamma, 'f', 2))
                     .arg(options.applyGamma ? "enabled" : "disabled")
                     .arg(options.legalRange ? "legal range" : "full range"));
}

void
WindowPrivate::init()
{
    QWidget* centralWidget = new QWidget(d.window);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    QLabel* titleLabel = new QLabel("DeckLink Rec.709 Test Generator", centralWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 5);
    titleLabel->setFont(titleFont);

    QLabel* infoLabel = new QLabel(
        "Generates 10-bit YUV Rec.709 patterns and continuously sends them to the UltraStudio / DeckLink output.",
        centralWidget);
    infoLabel->setWordWrap(true);

    QGroupBox* outputBox = new QGroupBox("Output", centralWidget);
    QVBoxLayout* outputLayout = new QVBoxLayout(outputBox);
    outputLayout->setSpacing(10);

    d.modeCombo = new QComboBox(outputBox);
    d.modeCombo->addItem("1080p25", QVariant::fromValue(int(bmdModeHD1080p25)));
    d.modeCombo->addItem("1080p24", QVariant::fromValue(int(bmdModeHD1080p24)));
    d.modeCombo->addItem("1080p2398", QVariant::fromValue(int(bmdModeHD1080p2398)));
    d.modeCombo->addItem("1080i50", QVariant::fromValue(int(bmdModeHD1080i50)));

    d.patternCombo = new QComboBox(outputBox);
    d.patternCombo->addItem("Color bars 75%");
    d.patternCombo->addItem("Rec.709 patches");
    d.patternCombo->addItem("Gray ramp");
    d.patternCombo->addItem("Gray ramp + center white");
    d.patternCombo->addItem("Gray ramp + 3 white patches");
    d.patternCombo->addItem("PLUGE");
    d.patternCombo->setCurrentIndex(3);

    d.gammaSpin = new QDoubleSpinBox(outputBox);
    d.gammaSpin->setDecimals(2);
    d.gammaSpin->setRange(1.00, 3.00);
    d.gammaSpin->setSingleStep(0.10);
    d.gammaSpin->setValue(2.40);
    d.gammaSpin->setFixedWidth(90);

    d.gammaCheck = new QCheckBox("Apply gamma encoding", outputBox);
    d.gammaCheck->setChecked(false);

    d.legalRangeCheck = new QCheckBox("Legal video range", outputBox);
    d.legalRangeCheck->setChecked(true);

    auto addRow = [&](const QString& label, QWidget* widget) {
        QWidget* row = new QWidget(outputBox);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(10);

        QLabel* title = new QLabel(label, row);
        title->setFixedWidth(100);

        layout->addWidget(title);
        layout->addWidget(widget, 1);
        outputLayout->addWidget(row);
    };

    addRow("Mode", d.modeCombo);
    addRow("Pattern", d.patternCombo);
    addRow("Gamma", d.gammaSpin);
    outputLayout->addWidget(d.gammaCheck);
    outputLayout->addWidget(d.legalRangeCheck);

    QWidget* buttonsRow = new QWidget(centralWidget);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsRow);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(8);

    QPushButton* sendButton = new QPushButton("Send Pattern", buttonsRow);
    QPushButton* stopButton = new QPushButton("Stop Output", buttonsRow);
    QPushButton* gamma22Button = new QPushButton("2.2", buttonsRow);
    QPushButton* gamma24Button = new QPushButton("2.4", buttonsRow);

    buttonsLayout->addWidget(sendButton);
    buttonsLayout->addWidget(stopButton);
    buttonsLayout->addSpacing(16);
    buttonsLayout->addWidget(gamma22Button);
    buttonsLayout->addWidget(gamma24Button);
    buttonsLayout->addStretch();

    QFrame* statusFrame = new QFrame(centralWidget);
    statusFrame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout* statusLayout = new QVBoxLayout(statusFrame);
    statusLayout->setContentsMargins(10, 8, 10, 8);

    d.statusLabel = new QLabel("Ready.", statusFrame);
    d.statusLabel->setWordWrap(true);
    statusLayout->addWidget(d.statusLabel);

    d.outputTimer = new QTimer(this);
    d.outputTimer->setTimerType(Qt::PreciseTimer);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(outputBox);
    mainLayout->addWidget(buttonsRow);
    mainLayout->addWidget(statusFrame);
    mainLayout->addStretch();

    QObject::connect(d.outputTimer, &QTimer::timeout, this, [this]() {
        resendFrame();
    });

    QObject::connect(sendButton, &QPushButton::clicked, this, [this]() {
        sendPattern();
    });

    QObject::connect(stopButton, &QPushButton::clicked, this, [this]() {
        stopOutput();
    });

    QObject::connect(gamma22Button, &QPushButton::clicked, this, [this]() {
        setGammaAndSend(2.20);
    });

    QObject::connect(gamma24Button, &QPushButton::clicked, this, [this]() {
        setGammaAndSend(2.40);
    });

    QObject::connect(d.gammaSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) {
        sendPattern();
    });

    QObject::connect(d.gammaCheck, &QCheckBox::toggled, this, [this](bool) {
        sendPattern();
    });

    QObject::connect(d.patternCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        sendPattern();
    });

    QObject::connect(d.legalRangeCheck, &QCheckBox::toggled, this, [this](bool) {
        sendPattern();
    });

    QObject::connect(d.modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        sendPattern();
    });

    d.window->setWindowTitle("testdecklink");
    d.window->setCentralWidget(centralWidget);
    d.window->resize(700, 360);
}

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}

} // namespace flipman
