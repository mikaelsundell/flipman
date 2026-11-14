// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "commands/commandtimeline.h"
#include "core/commandstack.h"
#include "core/session.h"
#include "playbackview.h"
#include "ui_playbackview.h"

#include <QFileDialog>
#include <QPointer>
#include <QSignalBlocker>
#include <algorithm>

namespace flipman {

class PlaybackViewPrivate {
public:
    void refresh();

    struct Data {
        Ui::PlaybackView ui;
        QPointer<PlaybackView> view;
    };
    Data d;
};

void
PlaybackViewPrivate::refresh()
{
    if (!flipman::session())
        return;

    auto* t = flipman::session()->timeline();
    const QSignalBlocker frame(d.ui.frameField), fps(d.ui.fps), loop(d.ui.loopButton), drop(d.ui.dropFrame),
        every(d.ui.everyFrame);
    d.ui.frameField->setMaximum(int(std::clamp<qint64>(flipman::session()->frameCount(), 1, 2147483646)));
    d.ui.frameField->setValue(int(t->time().frames() + 1));
    d.ui.timecodeLabel->setText(flipman::session()->isLoaded() ? t->timeCode().toString() : "00:00:00:00");
    d.ui.playButton->setText(t->isPlaying() ? "Pause" : "Play");
    d.ui.loopButton->setChecked(t->loop());
    d.ui.dropFrame->setChecked(t->fps().dropFrame());
    d.ui.everyFrame->setChecked(flipman::session()->everyFrame());
    int index = -1;
    for (int i = 0; i < d.ui.fps->count(); ++i)
        if (std::abs(d.ui.fps->itemData(i).toDouble() - t->fps().real()) < 0.001)
            index = i;
    if (index < 0) {
        d.ui.fps->addItem(QString::number(t->fps().real(), 'f', 3), t->fps().real());
        index = d.ui.fps->count() - 1;
    }
    d.ui.fps->setCurrentIndex(index);
    const bool enabled = flipman::session()->isLoaded() && !flipman::session()->isLoading();
    for (QWidget* control :
         QList<QWidget*> { static_cast<QWidget*>(d.ui.startButton), d.ui.previousButton, d.ui.nextButton,
                           d.ui.playButton, d.ui.endButton, d.ui.inButton, d.ui.outButton, d.ui.frameField })
        control->setEnabled(enabled);
}

PlaybackView::PlaybackView(QWidget* parent)
    : QWidget(parent)
    , p(new PlaybackViewPrivate)
{
    p->d.view = this;
    p->d.ui.setupUi(this);
    setAttribute(Qt::WA_StyledBackground);
    for (double rate : { 23.976, 24.0, 25.0, 29.97, 30.0, 47.952, 48.0, 50.0, 59.94, 60.0 })
        p->d.ui.fps->addItem(QString::number(rate, 'g', 6), rate);
    bindSession();
}

PlaybackView::~PlaybackView() = default;

void
PlaybackView::bindSession()
{
    auto* session = flipman::session();
    auto& ui = p->d.ui;
    connect(session, &Session::timelineChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::playbackChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::mediaChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::loadingChanged, this, [this]() { p->refresh(); });
    connect(ui.openButton, &QPushButton::clicked, this, [session, this]() {
        const auto file = QFileDialog::getOpenFileName(this, "Open movie or image");
        if (!file.isEmpty())
            session->openFile(file);
    });
    connect(ui.playButton, &QPushButton::clicked, session, &Session::togglePlaying);
    auto seek = [session](qint64 frame) {
        if (!session->isLoaded())
            return;

        frame = std::clamp<qint64>(frame, 0, session->frameCount() - 1);
        const auto after = sdk::av::Time::fromFrames(frame, session->timeline()->fps());
        if (after != session->timeline()->time())
            session->commands()->execute(timeCommand(session->timeline()->time(), after));
    };
    connect(ui.startButton, &QPushButton::clicked, this, [seek]() { seek(0); });
    connect(ui.endButton, &QPushButton::clicked, this, [seek, session]() { seek(session->frameCount() - 1); });
    connect(ui.previousButton, &QPushButton::clicked, this,
            [seek, session]() { seek(session->timeline()->time().frames() - 1); });
    connect(ui.nextButton, &QPushButton::clicked, this,
            [seek, session]() { seek(session->timeline()->time().frames() + 1); });
    connect(ui.frameField, &QSpinBox::editingFinished, this, [this, seek]() { seek(p->d.ui.frameField->value() - 1); });
    connect(ui.loopButton, &QPushButton::toggled, this, [session](bool loop) { session->timeline()->setLoop(loop); });
    connect(ui.everyFrame, &QCheckBox::toggled, session, &Session::setEveryFrame);
    auto setFps = [this, session]() {
        auto guessed = sdk::av::Fps::guess(p->d.ui.fps->currentData().toDouble());
        const sdk::av::Fps fps(qint32(guessed.numerator()), guessed.denominator(), p->d.ui.dropFrame->isChecked());
        if (fps != session->timeline()->fps())
            session->commands()->execute(fpsCommand(session->timeline()->fps(), fps));
    };
    connect(ui.fps, qOverload<int>(&QComboBox::currentIndexChanged), this, setFps);
    connect(ui.dropFrame, &QCheckBox::toggled, this, setFps);
    connect(ui.inButton, &QPushButton::clicked, session, &Session::setInPoint);
    connect(ui.outButton, &QPushButton::clicked, session, &Session::setOutPoint);
    connect(ui.fullscreenButton, &QPushButton::clicked, this, [this]() {
        if (window()->isFullScreen())
            window()->showNormal();
        else
            window()->showFullScreen();
    });
    p->refresh();
}

}  // namespace flipman
