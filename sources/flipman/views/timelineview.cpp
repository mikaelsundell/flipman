// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "commands/commandtimeline.h"
#include "core/commandstack.h"
#include "core/session.h"
#include "timelineview.h"
#include "ui_timelineview.h"

#include <QPointer>
#include <QSignalBlocker>

namespace flipman {

class TimelineViewPrivate {
public:
    void refresh();

    struct Data {
        Ui::TimelineView ui;
        sdk::av::Time before;
        bool scrubbing = false;
        QPointer<TimelineView> view;
    };
    Data d;
};

void
TimelineViewPrivate::refresh()
{
    if (!flipman::session())
        return;

    auto* t = flipman::session()->timeline();
    if (!d.scrubbing) {
        d.ui.scrubLine->setTime(t->time());
        d.ui.scrubLine->setRange(t->timeRange());
    }
    d.ui.scrubLine->setEnabled(flipman::session()->isLoaded() && !flipman::session()->isLoading());
    const auto range = t->ioRange();
    d.ui.inValue->setText(range.isValid() ? sdk::av::SmpteTime(range.start()).toString() : "00:00:00:00");
    d.ui.outValue->setText(
        range.isValid() ? sdk::av::SmpteTime(sdk::av::Time::fromFrames(range.end().frames() - 1, t->fps())).toString()
                        : "00:00:00:00");
    d.ui.totalFrames->setText(QString("%1 frames").arg(flipman::session()->frameCount()));
}

TimelineView::TimelineView(QWidget* parent)
    : QWidget(parent)
    , p(new TimelineViewPrivate)
{
    p->d.view = this;
    p->d.ui.setupUi(this);
    setAttribute(Qt::WA_StyledBackground);
    p->d.ui.scrubLine->setTimeCode(sdk::widgets::ScrubLine::SMPTE);
    p->d.ui.scrubLine->setTracking(true);
    bindSession();
}

TimelineView::~TimelineView() = default;

void
TimelineView::bindSession()
{
    auto* session = flipman::session();
    connect(session, &Session::timelineChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::playbackChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::mediaChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::loadingChanged, this, [this]() { p->refresh(); });
    connect(p->d.ui.scrubLine, &sdk::widgets::ScrubLine::sliderPressed, this, [this, session]() {
        p->d.before = session->timeline()->time();
        p->d.scrubbing = true;
        session->setFrame(p->d.ui.scrubLine->time().frames());
    });
    connect(p->d.ui.scrubLine, &sdk::widgets::ScrubLine::sliderMoved, this,
            [session](const sdk::av::Time& time) { session->setFrame(time.frames()); });
    connect(p->d.ui.scrubLine, &sdk::widgets::ScrubLine::sliderReleased, this, [this, session]() {
        p->d.scrubbing = false;
        const auto after = session->timeline()->time();
        if (after != p->d.before)
            session->commands()->execute(timeCommand(p->d.before, after), CommandStack::ExecutionMode::Applied);
        p->refresh();
    });
    p->refresh();
}

}  // namespace flipman
