// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "window.h"
#include "timelinewidget.h"
#include <QBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSlider>
#include <av/timeline.h>
#include <av/timerange.h>

class WindowPrivate : public QObject {
    Q_OBJECT
public:
    WindowPrivate();
    void init();
    QColor randomcolor() const;
    av::TimeRange randomtimerange(const av::Fps& fps) const;

public Q_SLOTS:
    void test();

public:
    struct Data {
        QPointer<av::Timeline> timeline;
        QPointer<TimelineWidget> timelineWidget;
        QPointer<Window> window;
    };
    Data d;
};

WindowPrivate::WindowPrivate() {}

void
WindowPrivate::init()
{
    av::Fps fps = av::Fps::fps_24();
    av::Time start = av::Time(static_cast<qreal>(0), fps);
    av::Time duration = av::Time(static_cast<qreal>(30), fps);
    av::TimeRange timerange(start, duration);

    d.timeline = new av::Timeline(d.window.data());
    d.timeline->set_timerange(timerange);

    for (int t = 0; t < 10; t++) {
        av::Track* track = new av::Track(d.timeline.data());
        track->set_name(QString("Video %1").arg(t));
        track->set_color(randomcolor());

        for (int c = 0; c < 10; c++) {
            av::Clip* clip = new av::Clip(d.timeline.data());
            clip->set_name(QString("Clip %1").arg(c));
            clip->set_color(randomcolor());
            track->insert_clip(clip, randomtimerange(fps));
        }
        d.timeline->insert_track(track);
    }

    // Create TimelineWidget
    d.timelineWidget = new TimelineWidget(d.window.data());
    d.timelineWidget->set_timeline(d.timeline);

    // Create QLabel for Time Display
    QLabel* timeLabel = new QLabel("00:00:00", d.window.data());
    timeLabel->setFixedWidth(100);  // Fixed width for consistent layout
    timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Create QSlider for Zoom Control
    QSlider* zoomSlider = new QSlider(Qt::Horizontal, d.window.data());
    zoomSlider->setRange(0, 100);  // Increase the range for finer control
    zoomSlider->setValue(50);      // Start at the middle position
    zoomSlider->setTickInterval(10);
    zoomSlider->setTickPosition(QSlider::TicksBelow);


    // Connect TimelineWidget time_changed Signal to QLabel
    connect(d.timelineWidget, &TimelineWidget::time_changed, [timeLabel](const av::Time& time) {
        int totalSeconds = static_cast<int>(time.seconds());
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        timeLabel->setText(QString("%1:%2:%3")
                               .arg(hours, 2, 10, QChar('0'))
                               .arg(minutes, 2, 10, QChar('0'))
                               .arg(seconds, 2, 10, QChar('0')));
    });

    // Connect QSlider to TimelineWidget Zoom
    connect(zoomSlider, &QSlider::valueChanged, [this](int value) {
        // Map slider to a non-linear zoom scale
        // Exponential mapping: value 0-100 to zoom 0.25 to 8.0
        double zoom = std::pow(2.0,
                               (value - 50) / 25.0);  // More aggressive zooming
        d.timelineWidget->set_zoom(zoom);
    });


    // Add TimelineWidget to QScrollArea for Horizontal Scrolling
    QScrollArea* scrollArea = new QScrollArea(d.window.data());
    scrollArea->setWidget(d.timelineWidget);
    scrollArea->setWidgetResizable(false);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* controlsWidget = new QWidget(d.window.data());
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(10);

    controlsLayout->addWidget(timeLabel);
    controlsLayout->addStretch();  // Horizontal Spacer
    controlsLayout->addWidget(zoomSlider);

    controlsWidget->setLayout(controlsLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->addWidget(controlsWidget);  // Add Controls at the Top
    mainLayout->addWidget(scrollArea);      // Timeline Scroll Area Below

    QWidget* centralWidget = new QWidget(d.window.data());
    centralWidget->setLayout(mainLayout);

    d.window->setCentralWidget(centralWidget);
    d.window->resize(800, 600);
}


QColor
WindowPrivate::randomcolor() const
{
    int red = QRandomGenerator::global()->bounded(256);
    int green = QRandomGenerator::global()->bounded(256);
    int blue = QRandomGenerator::global()->bounded(256);
    return QColor(red, green, blue, 255);
}

av::TimeRange
WindowPrivate::randomtimerange(const av::Fps& fps) const
{
    qreal start = QRandomGenerator::global()->bounded(30.0);
    qreal duration = QRandomGenerator::global()->bounded(30);
    av::Time startTime = av::Time(start, fps);
    av::Time durationTime = av::Time(duration, fps);
    return av::TimeRange(startTime, durationTime);
}

void
WindowPrivate::test()
{}

#include "window.moc"

Window::Window(QWidget* parent)
    : QMainWindow(parent)
    , p(new WindowPrivate())
{
    p->d.window = this;
    p->init();
}

Window::~Window() {}
