// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"

#include <flipmansdk/av/media.h>
#include <flipmansdk/core/application.h>
#include <flipmansdk/core/environment.h>
#include <flipmansdk/core/log.h>
#include <flipmansdk/plugins/imageeffectreader.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/widgets/viewer.h>

#include <QBoxLayout>
#include <QCoreApplication>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSlider>

namespace flipman {

class WindowPrivate : public QObject {
public:
    WindowPrivate();
    void init();

    struct Data {
        QString inputFile;
        QPointer<Window> window;
        QPointer<sdk::widgets::Viewer> viewer;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

void
WindowPrivate::init()
{
    const QStringList args = QCoreApplication::arguments();
    if (args.size() < 2)
        qFatal("No input file provided in arguments.");

    d.inputFile = args.at(1);

    const QString dataPath = sdk::core::Environment::resourcePath("../../../data");
#if (0)
    // rgb
    const QString filename = "23.967.00086400.exr";
    sdk::core::File file(QString("%1/exr/%2").arg(dataPath).arg(filename));
#else
    // nv12
    const QString filename = "square export 23.976 512x512.mov";
    sdk::core::File file(QString("%1/quicktime/%2").arg(sdk::core::Environment::resourcePath(dataPath)).arg(filename));
#endif

    Q_ASSERT_X(file.exists(), "RenderEngine::loadFile", "file does not exist");

    sdk::av::Media media;
    Q_ASSERT(media.open(file) && media.waitForOpened() && "could not open media");
    Q_ASSERT(media.isValid() && "error open media");

    media.read();
    sdk::core::ImageBuffer image = media.image();
    Q_ASSERT(image.isValid() && "image not valid");

    QWidget* centralWidget = new QWidget(d.window);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    QWidget* controlsWidget = new QWidget();
    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(8);

    QPushButton* fitButton = new QPushButton("Fit");
    fitButton->setShortcut(QKeySequence(Qt::Key_F));

    QPushButton* z25Button = new QPushButton("25%");
    z25Button->setShortcut(QKeySequence(Qt::Key_1));

    QPushButton* z50Button = new QPushButton("50%");
    z50Button->setShortcut(QKeySequence(Qt::Key_2));

    QPushButton* z75Button = new QPushButton("75%");
    z75Button->setShortcut(QKeySequence(Qt::Key_3));

    QPushButton* z100Button = new QPushButton("100%");
    z100Button->setShortcut(QKeySequence(Qt::Key_4));

    QLabel* zoomLabel = new QLabel("100%");
    zoomLabel->setMinimumWidth(60);
    zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    controlsLayout->addWidget(fitButton);
    controlsLayout->addWidget(z25Button);
    controlsLayout->addWidget(z50Button);
    controlsLayout->addWidget(z75Button);
    controlsLayout->addWidget(z100Button);
    controlsLayout->addStretch();
    controlsLayout->addWidget(zoomLabel);
    mainLayout->addWidget(controlsWidget);

    QFrame* viewerFrame = new QFrame();
    viewerFrame->setFrameShape(QFrame::NoFrame);
    viewerFrame->setObjectName("viewerFrame");
    viewerFrame->setStyleSheet("#viewerFrame {"
                               "  border: 1px solid #3a3a3a;"
                               "  background: #1e1e1e;"
                               "}");
    QVBoxLayout* frameLayout = new QVBoxLayout(viewerFrame);
    frameLayout->setContentsMargins(0, 0, 0, 0);

    d.viewer = new sdk::widgets::Viewer();
    d.viewer->setResolution(QSize(1920, 1080));

    frameLayout->addWidget(d.viewer);
    mainLayout->addWidget(viewerFrame);

    QSlider* timelineSlider = new QSlider(Qt::Horizontal);
    timelineSlider->setRange(0, 100);
    timelineSlider->setValue(0);
    timelineSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    timelineSlider->setFixedHeight(28);

    mainLayout->addWidget(timelineSlider);

    sdk::render::ImageLayer imageLayer;
    imageLayer.setImage(image);

    QString shader = "fx/warm.fx";
    sdk::core::File shaderFile(QString("%1/%2").arg(dataPath).arg(shader));
    Q_ASSERT(shaderFile.exists() && "fx file does not exist");

    QScopedPointer<sdk::plugins::ImageEffectReader> reader(
        sdk::core::pluginRegistry()->getPlugin<sdk::plugins::ImageEffectReader>(shaderFile.extension()));

    Q_ASSERT(reader && "no reader for fx extension");
    Q_ASSERT(reader->open(shaderFile) && "could not open reader for fx");

    sdk::render::ImageEffect imageEffect = reader->imageEffect();
    Q_ASSERT(imageEffect.isValid() && "image effect is not valid");

    imageLayer.setImageEffect(imageEffect);
    d.viewer->setImageLayers({ imageLayer });
    d.viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    connect(fitButton, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoomMode(sdk::widgets::Viewer::FitToView);
    });

    connect(z25Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(0.25f);
    });

    connect(z50Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(0.5f);
    });

    connect(z75Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(0.75f);
    });

    connect(z100Button, &QPushButton::clicked, this, [this]() {
        if (d.viewer)
            d.viewer->setZoom(1.0f);
    });

    connect(d.viewer, &sdk::widgets::Viewer::zoomChanged, zoomLabel, [zoomLabel](float zoom) {
        int percent = int(std::round(zoom * 100.0f));
        zoomLabel->setText(QString("%1%").arg(percent));
    });

    d.window->setWindowTitle("testview");
    d.window->setCentralWidget(centralWidget);
    d.window->resize(800, 600);
}

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}

}  // namespace flipman
