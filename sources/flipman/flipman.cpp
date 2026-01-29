// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include "flipman.h"

#include <flipmansdk/av/media.h>
#include <flipmansdk/av/timeline.h>
#include <flipmansdk/core/platform.h>
#include <flipmansdk/widgets/renderwidget.h>
#include <flipmansdk/widgets/timeline.h>

#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>

#include <QtConcurrent>
#include <QtGlobal>

// generated files
#include "ui_flipman.h"

class FlipmanPrivate : public QObject {
    Q_OBJECT
public:
    FlipmanPrivate();
    void init();
    bool eventFilter(QObject* object, QEvent* event);

public Q_SLOTS:
    void open();
    void seek(av::Time time);
    void seek_start();
    void seek_previous();
    void seek_previous10();
    void seek_next();
    void seek_next10();
    void seek_end();
    void seek_frame(qint64 frame);
    void seek_time(const av::Time& time);
    void seek_finished();
    void play(bool checked);
    void stop();
    void set_opened(const QString& filename);
    void set_video(const QImage& image);
    void set_audio(const QByteArray& buffer);
    void set_time(const av::Time& time);
    void set_timecode(const av::Time& time);
    void set_actual_fps(float fps);
    void fullscreen(bool checked);
    void loop(bool checked);
    void everyframe(bool everyframe);
    void frames();
    void time();
    void smpte();

    void power(core::Platform::Power power);
    void stayawake(bool checked);
    void showinfinder();
    void debug();

public:
    void run_open(const QString& filename)
    {
        /*
            if (!future.isRunning()) {
                future = QtConcurrent::run([&, filename] {
                    timeline->open(filename);
                    timeline->read();
                    timeline->seek(stream->range().start()); // todo: make sure we reset after read()
                });
            } else {
                qWarning() << "could not open reader, thread already running";
            }
             */
    }
    void run_seek(av::Time time)
    {
        if (!d.future.isRunning()) {
            d.future = QtConcurrent::run([&, time] { d.timeline->seek(time); });
            d.watcher.setFuture(d.future);
        }
        d.seek = time;
    }
    void run_play()
    {
        if (!d.future.isRunning()) {
            d.future = QtConcurrent::run([&] { d.timeline->play(); });
        }
        else {
            qWarning() << "could not run play, thread already running";
        }
    }
    void run_stop()
    {
        d.timeline->stop();
        if (d.future.isRunning()) {
            d.future.waitForFinished();
        }
        else {
            qWarning() << "could not wait for finished, thread is not running";
        }
    }

public:
    struct Data {
        bool loop;
        bool everyframe;
        bool play;
        bool fullscreen;
        bool ready = false;
        av::Time seek;
        QFuture<void> future;
        QFutureWatcher<void> watcher;
        QSharedPointer<av::Media> media;
        QScopedPointer<av::Timeline> timeline;
        QScopedPointer<core::Platform> platform;
    };
    Data d;
    QStringList arguments;
    QScopedPointer<Ui_Flipman> ui;
    QPointer<Flipman> window;
};

FlipmanPrivate::FlipmanPrivate() {}

void
FlipmanPrivate::init()
{
    d.platform.reset(new core::Platform());
    // ui
    ui.reset(new Ui_Flipman());
    ui->setupUi(window.data());
    window->setFocus();
    window->installEventFilter(this);
    // timeline
    d.timeline.reset(new av::Timeline());
    // connect
    connect(ui->menu_open, &QAction::triggered, this, &FlipmanPrivate::open);
    connect(ui->menu_start, &QAction::triggered, this, &FlipmanPrivate::seek_start);
    connect(ui->menu_previous, &QAction::triggered, this, &FlipmanPrivate::seek_previous);
    connect(ui->menu_previous10, &QAction::triggered, this, &FlipmanPrivate::seek_previous10);
    connect(ui->menu_play, &QAction::triggered, this, &FlipmanPrivate::play);
    connect(ui->menu_next, &QAction::triggered, this, &FlipmanPrivate::seek_next);
    connect(ui->menu_next10, &QAction::triggered, this, &FlipmanPrivate::seek_next10);
    connect(ui->menu_end, &QAction::triggered, this, &FlipmanPrivate::seek_end);
    connect(ui->menu_loop, &QAction::triggered, this, &FlipmanPrivate::loop);
    connect(ui->menu_fullscreen, &QAction::triggered, this, &FlipmanPrivate::fullscreen);
    connect(ui->tool_open, &QPushButton::pressed, this, &FlipmanPrivate::open);
    connect(ui->tool_start, &QPushButton::pressed, this, &FlipmanPrivate::seek_start);
    connect(ui->tool_previous, &QPushButton::pressed, this, &FlipmanPrivate::seek_previous);
    connect(ui->tool_play, &QPushButton::toggled, this, &FlipmanPrivate::play);
    connect(ui->tool_next, &QPushButton::pressed, this, &FlipmanPrivate::seek_next);
    connect(ui->tool_end, &QPushButton::pressed, this, &FlipmanPrivate::seek_end);
    connect(ui->tool_loop, &QPushButton::toggled, this, &FlipmanPrivate::loop);
    connect(ui->tool_everyframe, &QCheckBox::clicked, this, &FlipmanPrivate::everyframe);
    connect(ui->tool_fullscreen, &QPushButton::toggled, this, &FlipmanPrivate::fullscreen);
    connect(ui->timeline_frames, &QAction::triggered, this, &FlipmanPrivate::frames);
    connect(ui->timeline_time, &QAction::triggered, this, &FlipmanPrivate::time);
    connect(ui->timeline_smpte, &QAction::triggered, this, &FlipmanPrivate::smpte);
    {
        QActionGroup* actions = new QActionGroup(this);
        actions->setExclusive(true);
        {
            actions->addAction(ui->timeline_frames);
            actions->addAction(ui->timeline_time);
            actions->addAction(ui->timeline_smpte);
        }
    }
    connect(ui->utils_showinfinder, &QAction::triggered, this, &FlipmanPrivate::showinfinder);
    // timeline
    connect(ui->timeline, &widgets::Timeline::slider_pressed, this, &FlipmanPrivate::stop);
    connect(ui->timeline, &widgets::Timeline::slider_moved, this, &FlipmanPrivate::seek_time);
    // status
    connect(ui->stayawake, &QCheckBox::clicked, this, &FlipmanPrivate::stayawake);
    // debug
    connect(ui->debug, &QCheckBox::clicked, this, &FlipmanPrivate::debug);
    // watchers
    connect(&d.watcher, &QFutureWatcher<void>::finished, this, &FlipmanPrivate::seek_finished);
    // timeline
    //connect(timeline.data(), &A::opened, this, &FlipmanPrivate::set_opened); // todo: fix from media instead?
    //connect(timeline.data(), &AVTimeline::video_changed, this, &FlipmanPrivate::set_video); // todo: fix?
    connect(d.timeline.data(), &av::Timeline::audio_changed, this, &FlipmanPrivate::set_audio);
    connect(d.timeline.data(), &av::Timeline::time_changed, this, &FlipmanPrivate::set_time);
    connect(d.timeline.data(), &av::Timeline::timecode_changed, this, &FlipmanPrivate::set_timecode);
    connect(d.timeline.data(), &av::Timeline::actualfps_changed, this, &FlipmanPrivate::set_actual_fps);
    connect(d.timeline.data(), &av::Timeline::play_changed, ui->menu_play, &QAction::setChecked);
    connect(d.timeline.data(), &av::Timeline::play_changed, ui->tool_play, &QPushButton::setChecked);
    connect(d.timeline.data(), &av::Timeline::loop_changed, ui->menu_loop, &QAction::setChecked);
    connect(d.timeline.data(), &av::Timeline::loop_changed, ui->tool_loop, &QPushButton::setChecked);
    connect(d.timeline.data(), &av::Timeline::time_changed, ui->timeline, &widgets::Timeline::set_time);
    // platform
    connect(d.platform.data(), &core::Platform::power_changed, this, &FlipmanPrivate::power);
}

bool
FlipmanPrivate::eventFilter(QObject* object, QEvent* event)
{
    static bool dragging = false;
    static QPointF position;
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mousevent = static_cast<QMouseEvent*>(event);
        if (mousevent->button() == Qt::LeftButton) {
            dragging = true;
            position = mousevent->globalPosition() - window->frameGeometry().topLeft();
            if (window->focusWidget()) {
                window->focusWidget()->clearFocus();
            }
            return true;
        }
    }
    else if (event->type() == QEvent::MouseMove) {
        if (dragging) {
            QMouseEvent* mousevent = static_cast<QMouseEvent*>(event);
            window->move((mousevent->globalPosition() - position).toPoint());
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        dragging = false;
        return true;
    }
    if (event->type() == QEvent::MouseButtonDblClick) {
        if (window->isMaximized()) {
            window->showNormal();
        }
        else {
            window->showMaximized();
        }
        return true;
    }
    if (event->type() == QEvent::Wheel) {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        int delta = wheelEvent->angleDelta().y();
        if (delta > 0) {
            seek_next();
        }
        else if (delta < 0) {
            seek_previous();
        }
        return true;
    }
    else if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (dragEvent->mimeData()->hasUrls()) {
            dragEvent->acceptProposedAction();
        }
        return true;
    }
    else if (event->type() == QEvent::Drop) {
        QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
        if (dropEvent->mimeData()->hasUrls()) {
            QList<QUrl> urls = dropEvent->mimeData()->urls();
            for (const QUrl& url : urls) {
                QString filepath = url.toLocalFile();
                if (!filepath.isEmpty()) {
                    /*
                    if (timeline->is_supported(QFileInfo(filepath).suffix())) {
                        stop();
                        run_open(filepath);
                    }
                    else {
                        qWarning() << "warning: file format not supported: " << filepath;
                    }*/
                }
            }
        }
        return true;
    }
    else if (event->type() == QEvent::Show) {
        if (!d.ready) {
            if (arguments.contains("--open")) {
                qsizetype index = arguments.indexOf("--open");
                if (index != -1 && index + 1 < arguments.size()) {
                    QString filepath = arguments.at(index + 1);
                    if (!filepath.isEmpty()) {
                        run_open(filepath);
                    }
                }
            }
            d.ready = true;
        }
        return true;
    }
    else if (event->type() == QEvent::Close) {
        stop();
        return true;
    }
    return QObject::eventFilter(object, event);
}

void
FlipmanPrivate::open()
{
    QString filename = QFileDialog::getOpenFileName(window.data(), tr("Open QuickTime Movie"), QDir::homePath(),
                                                    tr("QuickTime Movies (*.mov *.mp4);;All Files (*)"));
    if (!filename.isEmpty()) {
        stop();
        run_open(filename);
    }
}

void
FlipmanPrivate::seek(av::Time time)
{
    stop();
    if (d.timeline->time() != time) {
        run_seek(time);
    }
}

void
FlipmanPrivate::seek_start()
{
    seek(d.timeline->range().start());
}

void
FlipmanPrivate::seek_previous()
{
    seek_frame(-1);
}

void
FlipmanPrivate::seek_previous10()
{
    seek_frame(-10);
}

void
FlipmanPrivate::seek_next()
{
    seek_frame(1);
}

void
FlipmanPrivate::seek_next10()
{
    seek_frame(10);
}

void
FlipmanPrivate::seek_end()
{
    seek(d.timeline->range().end());
}

void
FlipmanPrivate::seek_frame(qint64 frame)
{
    stop();
    av::Time time = d.timeline->time();
    run_seek(av::Time(time.ticks(time.frames() + frame), time.timescale(), time.fps()));
}

void
FlipmanPrivate::seek_time(const av::Time& time)
{
    run_seek(time);
}

void
FlipmanPrivate::seek_finished()
{
    if (d.seek.is_valid()) {
        if (d.seek.frames() != d.timeline->time().frames()) {
            run_seek(d.seek);
        }
        d.seek.reset();
    }
}

void
FlipmanPrivate::play(bool checked)
{
    if (d.play != checked) {
        d.play = checked;
        if (d.play) {
            run_play();
        }
        else {
            d.timeline->stop();
        }
    }
}

void
FlipmanPrivate::stop()
{
    if (d.timeline->is_playing()) {
        run_stop();
    }
}

void
FlipmanPrivate::fullscreen(bool checked)
{
    if (d.fullscreen != checked) {
        QLayout* layout = qobject_cast<QVBoxLayout*>(ui->central_widget->layout());
        if (checked) {
            for (int i = 0; i < layout->count(); ++i) {
                QWidget* widget = layout->itemAt(i)->widget();
                if (widget && widget != ui->render_widget) {
                    widget->hide();
                }
            }
            window->showFullScreen();
        }
        else {
            for (int i = 0; i < layout->count(); ++i) {
                QWidget* widget = layout->itemAt(i)->widget();
                if (widget && widget != ui->render_widget) {
                    widget->show();
                }
            }
            window->showNormal();
        }
        d.fullscreen = checked;
    }
}

void
FlipmanPrivate::loop(bool checked)
{
    if (d.loop != checked) {
        d.timeline->set_loop(checked);
        d.loop = checked;
    }
}

void
FlipmanPrivate::everyframe(bool checked)
{
    if (d.everyframe != checked) {
        d.timeline->set_everyframe(checked);
        d.everyframe = checked;
    }
}

void
FlipmanPrivate::frames()
{
    ui->timeline->set_timecode(widgets::Timeline::Frames);
    ui->timecode->set_timecode(widgets::Timeedit::Frames);
    ui->timeline_start->set_timecode(widgets::Timeedit::Frames);
    ui->timeline_duration->set_timecode(widgets::Timeedit::Frames);
    set_time(d.timeline->time());
}

void
FlipmanPrivate::time()
{
    ui->timeline->set_timecode(widgets::Timeline::Time);
    ui->timecode->set_timecode(widgets::Timeedit::Time);
    ui->timeline_start->set_timecode(widgets::Timeedit::Time);
    ui->timeline_duration->set_timecode(widgets::Timeedit::Time);
    set_time(d.timeline->time());
}

void
FlipmanPrivate::smpte()
{
    ui->timeline->set_timecode(widgets::Timeline::SMPTE);
    ui->timecode->set_timecode(widgets::Timeedit::SMPTE);
    ui->timeline_start->set_timecode(widgets::Timeedit::SMPTE);
    ui->timeline_duration->set_timecode(widgets::Timeedit::SMPTE);
    set_time(d.timeline->time());
}

void
FlipmanPrivate::power(core::Platform::Power power)
{
    if (power == core::Platform::PowerOff || power == core::Platform::Restart || power == core::Platform::Sleep) {
        stop();
    }
}

void
FlipmanPrivate::stayawake(bool checked)
{
    if (checked) {
        d.platform->stayawake(checked);
    }
    else {
        d.platform->stayawake(checked);
    }
}

void
FlipmanPrivate::showinfinder()
{
    /* // todo: fix use media instead
    if (timeline->is_open()) {
        //QDesktopServices::openUrl(QUrl::fromLocalFile(timeline->filename()));
    }*/
}

void
FlipmanPrivate::debug()
{
    // todo: place holder for debug tests
}

void
FlipmanPrivate::set_opened(const QString& filename)
{
    /* // todo: fix
    if (timeline->error() == AVStream::NO_ERROR) {
        AVTimeRange range = timeline->range();
        ui->df->setChecked(timeline->fps().drop_frame());
        ui->fps->setValue(timeline->fps());
        ui->actual_fps->setText(QString::number(timeline->fps()));
        ui->playback_widget->setEnabled(true);
        ui->timeline_start->set_time(range.start());
        ui->timeline_duration->set_time(range.end());
        ui->timeline->set_time(timeline->time());
        ui->timeline->set_range(timeline->range());
        ui->timeline->setEnabled(true);
    }
    else {
        ui->status->setText(timeline->error_message());
        qWarning() << "warning: " << ui->status->text();
    }*/
}

void
FlipmanPrivate::set_video(const QImage& image)
{
    /*
    QString title = timeline->title();
    if (title.isEmpty()) {
        title = QFileInfo(timeline->filename()).fileName();
    }
    window->setWindowTitle(QString("Flipman: %1").arg(title));
    {
        int width = image.width();
        int height = image.height();
        QString format;
        switch (image.format()) {
            case QImage::Format_ARGB32: format = "ARGB"; break;
            case QImage::Format_RGB32: format = "RGB"; break;
            case QImage::Format_Grayscale8: format = "Grayscale"; break;
            case QImage::Format_RGB888: format = "RGB"; break;
            case QImage::Format_RGBA8888: format = "RGBA"; break;
            default: format = "Unknown"; break;
        }
        ui->info->setText(QString("%1x%2 %3 %4-bit").arg(width).arg(height).arg(format).arg(image.depth()));
    }
    ui->render_widget->set_image(image);*/

    // todo: switch to render layers etc
}

void
FlipmanPrivate::set_audio(const QByteArray& buffer)
{
    /* // todo: fix audio
    if (timeline->error() == AVStream::NO_ERROR) {
        // todo: we don't support audio streaming yet
    }
    else {
        ui->status->setText(stream->error_message());
        qWarning() << "warning: " << ui->status->text();
    }*/
}

void
FlipmanPrivate::set_time(const av::Time& time)
{
    ui->frame->setText(QString("%1").arg(time.frames(), 4, 10, QChar('0')));
    ui->timecode->set_time(time);
    ui->timeline_start->set_time(time);
    ui->timeline_duration->set_time(d.timeline->range().duration());
    ui->time_timecode->setText(av::SmpteTime(time).to_string());
}

void
FlipmanPrivate::set_timecode(const av::Time& time)
{
    ui->timeline_timecode->setText(av::SmpteTime(time).to_string());
    ui->timeline_frame->setText(QString::number(time.frames()));
}

void
FlipmanPrivate::set_actual_fps(float fps)
{
    if (fps < d.timeline->fps()) {
        ui->actual_fps->setText(QString("*%1").arg(QString::number(fps, 'f', 3)));
    }
    else {
        ui->actual_fps->setText(QString("%1").arg(QString::number(fps, 'f', 3)));
    }
}

#include "flipman.moc"

Flipman::Flipman(QWidget* parent)
    : QMainWindow(parent)
    , p(new FlipmanPrivate())
{
    p->window = this;
    p->init();
}

Flipman::~Flipman() {}

void
Flipman::set_arguments(const QStringList& arguments)
{
    p->arguments = arguments;
}
