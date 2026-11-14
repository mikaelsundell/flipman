// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "app/application.h"
#include "commands/commanddisplay.h"
#include "core/commandstack.h"
#include "core/displayprofiles.h"
#include "core/session.h"
#include "displayview.h"
#include "private/colorselector.h"
#include "ui_displayview.h"

#include <QPointer>
#include <QSignalBlocker>
#include <memory>

namespace flipman {

class DisplayViewPrivate {
public:
    void refreshViews(const QString& display, const QString& selected);
    void refresh();
    void apply();

    struct Data {
        Ui::DisplayView ui;
        std::unique_ptr<ColorSelector> input;
        std::unique_ptr<ColorSelector> working;
        std::unique_ptr<ColorSelector> output;
        bool refreshing = false;
        QPointer<DisplayView> view;
    };
    Data d;
};

void
DisplayViewPrivate::refreshViews(const QString& display, const QString& selected)
{
    const QSignalBlocker blocker(d.ui.view);
    d.ui.view->clear();
    d.ui.view->addItem("Auto", "Un-tone-mapped");
    d.ui.view->insertSeparator(d.ui.view->count());
    for (const auto& name : sdk::render::ColorPipeline::views(display))
        if (name != "Raw")
            d.ui.view->addItem(name == "Un-tone-mapped" ? "Direct" : name, name);
    d.ui.view->insertSeparator(d.ui.view->count());
    d.ui.view->addItem("Raw", "Raw");
    int index = d.ui.view->findData(selected);
    d.ui.view->setCurrentIndex(index >= 0 ? index : 0);
}

void
DisplayViewPrivate::refresh()
{
    if (!flipman::session())
        return;

    d.refreshing = true;
    const auto color = flipman::session()->colorSettings();
    d.input->setAutoName(flipman::session()->autoInputColorSpace());
    d.input->select(flipman::session()->inputColorSpace(), false);
    d.working->setAutoName("ACEScg");
    d.working->select(color.workingSpace, false);
    d.output->setAutoName("Rec.1886 Rec.709 - Display");
    d.output->select(color.display, false);
    refreshViews(color.display, color.view);
    const QSignalBlocker blocker(d.ui.display);
    const auto tag = flipman::session()->displayTransform();
    const auto list = displayProfiles();
    for (int i = 0; i < list.size(); ++i)
        if (list[i].transform.colorSpace == tag.colorSpace
            && list[i].transform.transferFunction == tag.transferFunction)
            d.ui.display->setCurrentIndex(i);
    d.refreshing = false;
}

void
DisplayViewPrivate::apply()
{
    if (d.refreshing || !flipman::session())
        return;

    auto settings = flipman::session()->colorSettings();
    auto working = d.working->name();
    if (working.isEmpty())
        working = "ACEScg";
    settings.workingSpace = working;
    settings.display = d.output->name().isEmpty() ? QStringLiteral("Rec.1886 Rec.709 - Display") : d.output->name();
    settings.view = d.ui.view->currentData().toString();
    if (settings != flipman::session()->colorSettings() || d.input->name() != flipman::session()->inputColorSpace())
        flipman::session()->commands()->execute(colorCommand(flipman::session()->colorSettings(),
                                                             flipman::session()->inputColorSpace(), settings,
                                                             d.input->name(), flipman::session()->displayTransform()));
}

DisplayView::DisplayView(QWidget* parent)
    : QWidget(parent)
    , p(new DisplayViewPrivate)
{
    p->d.view = this;
    p->d.ui.setupUi(this);
    setAttribute(Qt::WA_StyledBackground);
    const auto profiles = sdk::render::ColorPipeline::colorProfiles();
    const auto workingNames = sdk::render::ColorPipeline::workingSpaces();
    QList<sdk::render::ColorProfile> working;
    for (const auto& profile : profiles)
        if (workingNames.contains(profile.name))
            working.append(profile);
    p->d.input = std::make_unique<ColorSelector>(p->d.ui.input, p->d.ui.inputTransfer, profiles, QString());
    p->d.working = std::make_unique<ColorSelector>(p->d.ui.working, p->d.ui.workingTransfer, working, "ACEScg");
    p->d.output = std::make_unique<ColorSelector>(p->d.ui.output, p->d.ui.outputTransfer, profiles,
                                                  "Rec.1886 Rec.709 - Display");
    for (const auto& entry : displayProfiles())
        p->d.ui.display->addItem(entry.name);
    bindSession();
}

DisplayView::~DisplayView() = default;

void
DisplayView::bindSession()
{
    auto* session = flipman::session();
    connect(session, &Session::mediaChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::colorChanged, this, [this]() { p->refresh(); });
    connect(session, &Session::displayChanged, this, [this]() { p->refresh(); });
    connect(p->d.input.get(), &ColorSelector::changed, this, [this]() { p->apply(); });
    connect(p->d.working.get(), &ColorSelector::changed, this, [this]() { p->apply(); });
    connect(p->d.output.get(), &ColorSelector::changed, this, [this]() {
        p->refreshViews(p->d.output->name(), p->d.ui.view->currentData().toString());
        p->apply();
    });
    connect(p->d.ui.view, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() { p->apply(); });
    connect(p->d.ui.display, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, session](int index) {
        if (p->d.refreshing || index < 0)
            return;

        session->commands()->execute(displayCommand(session->displayTransform(), displayProfiles()[index].transform));
    });
    p->refresh();
}

}  // namespace flipman
