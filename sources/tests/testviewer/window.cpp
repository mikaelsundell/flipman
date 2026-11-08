// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"
#include <QBoxLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <flipmansdk/av/media.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/log.h>
#include <flipmansdk/render/render.h>
#include <flipmansdk/widgets/viewer.h>
#include <limits>

namespace flipman {

class WindowPrivate : public QObject {
public:
    WindowPrivate();
    void init();
    bool loadFile(const QString& path);
    QString dropFile(const QMimeData* mime) const;
    void update();
    void updateTimeline();
    bool eventFilter(QObject* object, QEvent* event);
    void seekFrame(int frame);

public:
    struct DisplayTransform {
        QString label;
        sdk::render::DisplayTransform transform;
    };
    QVector<DisplayTransform> displayTransforms();

public:
    struct Data {
        QString inputFile;
        QPointer<Window> window;
        QPointer<sdk::render::RenderEngine> renderEngine;
        QPointer<sdk::widgets::Viewer> viewer;
        QPointer<QSlider> timelineSlider;
        QPointer<QLabel> timelineLabel;
        sdk::av::Media media;
        sdk::av::TimeRange timeRange;
        sdk::render::ImageLayer imageLayer;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

void
WindowPrivate::init()
{
    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1)
        d.inputFile = args.at(1);

    QWidget* centralWidget = new QWidget(d.window);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget* controlsWidget = new QWidget(centralWidget);
    controlsWidget->setFixedHeight(40);
    controlsWidget->setProperty("role", "toolbar");

    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(8, 0, 8, 0);
    controlsLayout->setSpacing(8);
    controlsLayout->setAlignment(Qt::AlignVCenter);

    QPushButton* fitButton = new QPushButton("Fit", controlsWidget);
    fitButton->setShortcut(QKeySequence(Qt::Key_F));

    QPushButton* z25Button = new QPushButton("25%", controlsWidget);
    z25Button->setShortcut(QKeySequence(Qt::Key_1));

    QPushButton* z50Button = new QPushButton("50%", controlsWidget);
    z50Button->setShortcut(QKeySequence(Qt::Key_2));

    QPushButton* z75Button = new QPushButton("75%", controlsWidget);
    z75Button->setShortcut(QKeySequence(Qt::Key_3));

    QPushButton* z100Button = new QPushButton("100%", controlsWidget);
    z100Button->setShortcut(QKeySequence(Qt::Key_4));

    QLabel* displayLabel = new QLabel("Display", controlsWidget);
    displayLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QComboBox* displayCombo = new QComboBox(controlsWidget);
    displayCombo->setMinimumWidth(220);

    const QVector<DisplayTransform> transforms = displayTransforms();
    for (int i = 0; i < transforms.size(); ++i)
        displayCombo->addItem(transforms[i].label, i);

    QLabel* zoomLabel = new QLabel("100%", controlsWidget);
    zoomLabel->setMinimumWidth(60);
    zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    controlsLayout->addWidget(fitButton, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z25Button, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z50Button, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z75Button, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(z100Button, 0, Qt::AlignVCenter);
    controlsLayout->addSpacing(16);
    controlsLayout->addWidget(displayLabel, 0, Qt::AlignVCenter);
    controlsLayout->addWidget(displayCombo, 0, Qt::AlignVCenter);
    controlsLayout->addStretch(1);
    controlsLayout->addWidget(zoomLabel, 0, Qt::AlignVCenter);

    mainLayout->addWidget(controlsWidget);

    QWidget* contentWidget = new QWidget(centralWidget);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(0);

    QFrame* viewerFrame = new QFrame(contentWidget);
    viewerFrame->setFrameShape(QFrame::NoFrame);

    QVBoxLayout* frameLayout = new QVBoxLayout(viewerFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);

    d.renderEngine = new sdk::render::RenderEngine(d.window);

    d.viewer = new sdk::widgets::Viewer(viewerFrame);
    d.viewer->setAcceptDrops(true);
    d.viewer->installEventFilter(this);
    d.viewer->setRenderEngine(d.renderEngine.data());
    d.viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const sdk::render::DisplayTransform displayTransform { sdk::render::ColorSpace::Rec709,
                                                           sdk::render::TransferFunction::Gamma24 };

    d.viewer->setDisplayTransform(displayTransform);

    int index = 0;
    for (int i = 0; i < transforms.size(); ++i) {
        if (transforms[i].transform.colorSpace == displayTransform.colorSpace
            && transforms[i].transform.transferFunction == displayTransform.transferFunction) {
            index = i;
            break;
        }
    }
    displayCombo->setCurrentIndex(index);

    frameLayout->addWidget(d.viewer);
    contentLayout->addWidget(viewerFrame, 1);

    QWidget* timelineWidget = new QWidget(contentWidget);
    QHBoxLayout* timelineLayout = new QHBoxLayout(timelineWidget);
    timelineLayout->setContentsMargins(8, 0, 8, 0);
    timelineLayout->setSpacing(8);

    d.timelineSlider = new QSlider(Qt::Horizontal, timelineWidget);

    d.timelineSlider->setRange(0, 0);
    d.timelineSlider->setEnabled(false);
    d.timelineSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    d.timelineSlider->setFixedHeight(28);

    d.timelineLabel = new QLabel(timelineWidget);
    d.timelineLabel->setMinimumWidth(140);
    d.timelineLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    timelineLayout->addWidget(d.timelineSlider, 1);
    timelineLayout->addWidget(d.timelineLabel);

    contentLayout->addWidget(timelineWidget);
    mainLayout->addWidget(contentWidget, 1);

    d.window->setAcceptDrops(true);
    d.window->installEventFilter(this);

    connect(fitButton, &QPushButton::clicked, this,
            [this]() { d.viewer->setZoomMode(sdk::widgets::Viewer::FitToView); });

    connect(z25Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(0.25f); });
    connect(z50Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(0.5f); });
    connect(z75Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(0.75f); });
    connect(z100Button, &QPushButton::clicked, this, [this]() { d.viewer->setZoom(1.0f); });

    connect(displayCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, displayCombo, transforms](int index) {
                if (index < 0)
                    return;

                const int transformIndex = displayCombo->itemData(index).toInt();
                if (transformIndex < 0 || transformIndex >= transforms.size())
                    return;

                const DisplayTransform& displayTransform = transforms[transformIndex];

                d.viewer->setDisplayTransform(
                    { displayTransform.transform.colorSpace, displayTransform.transform.transferFunction });

                d.viewer->update();
            });

    connect(d.viewer, &sdk::widgets::Viewer::zoomChanged, zoomLabel, [zoomLabel](float zoom) {
        const int percent = int(std::round(zoom * 100.0f));
        zoomLabel->setText(QString("%1%").arg(percent));
    });

    connect(d.timelineSlider, &QSlider::valueChanged, this, [this](int frame) { seekFrame(frame); });

    d.window->setWindowTitle("testviewer");
    d.window->setCentralWidget(centralWidget);
    d.window->resize(1200, 700);

    if (!d.inputFile.isEmpty())
        loadFile(d.inputFile);
    else {
        d.window->statusBar()->showMessage("Drop a movie or image here to open it");
        updateTimeline();
    }
}

bool
WindowPrivate::loadFile(const QString& path)
{
    // Decode before replacing the current media, so failed drops leave it usable.
    sdk::av::Media media;
    if (!media.open(sdk::core::File(path)) || !media.waitForOpened(5000) || !media.isValid()) {
        d.window->statusBar()->showMessage(QString("Could not open %1: %2").arg(path, media.error().message()));
        return false;
    }
    const auto time = media.read();
    const auto image = media.image();
    if (!time.isValid() || !image.isValid() || !image.isAllocated()) {
        d.window->statusBar()->showMessage(QString("Could not decode %1").arg(path));
        return false;
    }
    d.media = media;
    d.inputFile = path;
    d.timeRange = media.timeRange();
    d.imageLayer.setImage(image);
    d.renderEngine->setResolution(image.displayWindow().size());
    {
        const QSignalBlocker blocker(d.timelineSlider);
        const qint64 frameCount = d.timeRange.isValid() ? d.timeRange.duration().frames() : 1;
        const int lastFrame = int(std::clamp<qint64>(frameCount - 1, 0, std::numeric_limits<int>::max() - 1));
        d.timelineSlider->setRange(0, lastFrame);
        d.timelineSlider->setValue(0);
        d.timelineSlider->setEnabled(lastFrame > 0);
    }
    d.viewer->setZoomMode(sdk::widgets::Viewer::FitToView);
    d.window->setWindowTitle(QString("testviewer — %1").arg(QFileInfo(path).fileName()));
    d.window->statusBar()->clearMessage();
    updateTimeline();
    update();
    return true;
}

QString
WindowPrivate::dropFile(const QMimeData* mime) const
{
    if (!mime || !mime->hasUrls() || mime->urls().size() != 1)
        return {};
    const auto url = mime->urls().first();
    const QFileInfo file(url.toLocalFile());
    if (!url.isLocalFile() || !file.isFile() || !d.media.isSupported(file.suffix()))
        return {};
    return file.absoluteFilePath();
}

void
WindowPrivate::update()
{
    Q_ASSERT(d.viewer);
    d.renderEngine->setImageLayers({ d.imageLayer });
    d.viewer->update();
}

void
WindowPrivate::updateTimeline()
{
    Q_ASSERT(d.timelineLabel);

    const int frame = d.timelineSlider ? d.timelineSlider->value() : 0;
    const int total = d.media.isValid() ? d.timelineSlider->maximum() + 1 : 0;

    if (d.media.isValid() && d.media.time().isValid()) {
        d.timelineLabel->setText(QString("%1 / %2  %3").arg(frame + 1).arg(total).arg(d.media.time().toString()));
    }
    else {
        d.timelineLabel->setText(QString("%1 / %2").arg(total > 0 ? frame + 1 : 0).arg(total));
    }
}

bool
WindowPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (object == d.window || object == d.viewer) {
        if (event->type() == QEvent::DragEnter) {
            auto* drag = static_cast<QDragEnterEvent*>(event);
            if (!dropFile(drag->mimeData()).isEmpty() && drag->possibleActions().testFlag(Qt::CopyAction)) {
                drag->setDropAction(Qt::CopyAction);
                drag->accept();
            }
            else
                drag->ignore();
            return true;
        }
        if (event->type() == QEvent::Drop) {
            auto* drop = static_cast<QDropEvent*>(event);
            const auto path = dropFile(drop->mimeData());
            if (!path.isEmpty() && drop->possibleActions().testFlag(Qt::CopyAction) && loadFile(path)) {
                drop->setDropAction(Qt::CopyAction);
                drop->accept();
            }
            else
                drop->ignore();
            return true;
        }
    }
    if (object != d.window)
        return QObject::eventFilter(object, event);

    switch (event->type()) {
    case QEvent::ScreenChangeInternal:
    case QEvent::DevicePixelRatioChange:
    case QEvent::PlatformSurface:
        d.window->update();
        QTimer::singleShot(0, d.window, [this]() {
            if (d.viewer)
                d.viewer->update();
            if (d.window)
                d.window->update();
        });
        break;
    default: break;
    }
    return QObject::eventFilter(object, event);
}

void
WindowPrivate::seekFrame(int frame)
{
    if (!d.media.isValid() || !d.timeRange.isValid())
        return;

    const sdk::av::Time start = d.timeRange.start();
    const sdk::av::Time target(start, start.ticks() + start.ticks(frame));
    const sdk::av::Time duration(start, start.ticks(1));
    const sdk::av::TimeRange range(target, duration);

    const sdk::av::Time seekTime = d.media.seek(range);
    if (!seekTime.isValid()) {
        qWarning() << "seek failed:" << d.media.error();
        return;
    }

    const sdk::av::Time readTime = d.media.read();
    if (!readTime.isValid()) {
        qWarning() << "read after seek failed:" << d.media.error();
        return;
    }

    const sdk::core::ImageBuffer image = d.media.image();
    if (!image.isValid()) {
        qWarning() << "invalid image after seek:" << d.media.error();
        return;
    }

    d.imageLayer.setImage(image);

    updateTimeline();
    update();
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
    : QMainWindow(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}

bool
Window::isValid() const
{
    return p->d.media.isValid() && p->d.imageLayer.image().isAllocated();
}

}  // namespace flipman
