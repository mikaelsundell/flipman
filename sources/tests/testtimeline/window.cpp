// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"
#include "timelinewidget.h"

#include <flipmansdk/av/timeline.h>
#include <flipmansdk/av/timerange.h>

#include <QBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSlider>

namespace flipman {
class WindowPrivate : public QObject {
public:
    WindowPrivate();
    void init();
    QColor randomColor() const;
    sdk::av::TimeRange randomTimeRange(const sdk::av::Fps& fps) const;
    
public:
    struct Data {
        QPointer<sdk::av::Timeline> timeLine;
        QPointer<TimeLineWidget> timeLineWidget;
        QPointer<Window> window;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

void
WindowPrivate::init()
{
    sdk::av::Fps fps = sdk::av::Fps::fps24();
    sdk::av::Time start = sdk::av::Time(static_cast<qreal>(0), fps);
    sdk::av::Time duration = sdk::av::Time(static_cast<qreal>(30), fps);
    sdk::av::TimeRange timeRange(start, duration);
    
    d.timeLine = new sdk::av::Timeline(d.window.data());
    d.timeLine->setTimeRange(timeRange);
    
    for (int t = 0; t < 10; t++) {
        sdk::av::Track* track = new sdk::av::Track(d.timeLine.data());
        track->setName(QString("Video %1").arg(t));
        track->setColor(randomColor());
        
        for (int c = 0; c < 10; c++) {
            sdk::av::Clip* clip = new sdk::av::Clip(d.timeLine.data());
            clip->setName(QString("Clip %1").arg(c));
            clip->setColor(randomColor());
            track->insertClip(clip, randomTimeRange(fps));
        }
        d.timeLine->insertTrack(track);
    }
    
    d.timeLineWidget = new TimeLineWidget(d.window.data());
    d.timeLineWidget->setTimeLine(d.timeLine);
    
    QLabel* timeLabel = new QLabel("00:00:00", d.window.data());
    timeLabel->setFixedWidth(100);  // Fixed width for consistent layout
    timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    QSlider* zoomSlider = new QSlider(Qt::Horizontal, d.window.data());
    zoomSlider->setRange(0, 100);  // Increase the range for finer control
    zoomSlider->setValue(50);      // Start at the middle position
    zoomSlider->setTickInterval(10);
    zoomSlider->setTickPosition(QSlider::TicksBelow);
    
    connect(d.timeLineWidget, &TimeLineWidget::timeChanged, [timeLabel](const sdk::av::Time& time) {
        int totalSeconds = static_cast<int>(time.seconds());
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        timeLabel->setText(QString("%1:%2:%3")
                           .arg(hours, 2, 10, QChar('0'))
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0')));
    });
    connect(zoomSlider, &QSlider::valueChanged, [this](int value) {
        // map slider to a non-linear zoom scale
        // exponential mapping: value 0-100 to zoom 0.25 to 8.0
        double zoom = std::pow(2.0,
                               (value - 50) / 25.0);  // More aggressive zooming
        d.timeLineWidget->setZoom(zoom);
    });
    
    QScrollArea* scrollArea = new QScrollArea(d.window.data());
    scrollArea->setWidget(d.timeLineWidget);
    scrollArea->setWidgetResizable(false);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QWidget* controlsWidget = new QWidget(d.window.data());
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(10);
    
    controlsLayout->addWidget(timeLabel);
    controlsLayout->addStretch();
    controlsLayout->addWidget(zoomSlider);
    
    controlsWidget->setLayout(controlsLayout);
    
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(controlsWidget);
    mainLayout->addWidget(scrollArea);
    
    QWidget* centralWidget = new QWidget(d.window.data());
    centralWidget->setLayout(mainLayout);
    
    d.window->setCentralWidget(centralWidget);
    d.window->resize(800, 600);
}


QColor
WindowPrivate::randomColor() const
{
    int red = QRandomGenerator::global()->bounded(256);
    int green = QRandomGenerator::global()->bounded(256);
    int blue = QRandomGenerator::global()->bounded(256);
    return QColor(red, green, blue, 255);
}

sdk::av::TimeRange
WindowPrivate::randomTimeRange(const sdk::av::Fps& fps) const
{
    qreal start = QRandomGenerator::global()->bounded(30.0);
    qreal duration = QRandomGenerator::global()->bounded(30);
    sdk::av::Time startTime = sdk::av::Time(start, fps);
    sdk::av::Time durationTime = sdk::av::Time(duration, fps);
    return sdk::av::TimeRange(startTime, durationTime);
}

Window::Window(QWidget* parent)
: QMainWindow(parent)
, p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}
}
