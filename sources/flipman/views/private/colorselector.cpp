// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "colorselector.h"

#include <QMap>
#include <algorithm>

namespace flipman {

ColorSelector::ColorSelector(QPushButton* gamut, QComboBox* encoding, const QList<sdk::render::ColorProfile>& profiles,
                             const QString& initial)
    : QObject(gamut)
{
    d.gamut = gamut;
    d.encoding = encoding;
    d.profiles = profiles;
    auto* menu = new QMenu(gamut);
    d.actions = new QActionGroup(menu);
    d.actions->setExclusive(true);
    const auto action = [this](QMenu* parent, const QString& label, const QString& name) {
        auto* item = parent->addAction(label);
        item->setCheckable(true);
        item->setData(name);
        item->setToolTip(name);
        d.actions->addAction(item);
        return item;
    };
    action(menu, "Auto", {});
    menu->addSeparator();
    QMap<QString, QMenu*> groups;
    QMap<QString, QStringList> labels;
    for (const auto& profile : profiles) {
        if (profile.name == "Raw")
            continue;
        auto family = profile.family;
        if (family.isEmpty())
            family = "Other";
        auto* parent = menu;
        QString path;
        for (const auto& part : family.split('/') + QStringList { profile.gamut }) {
            path += '/' + part;
            if (!groups.contains(path))
                groups.insert(path, parent->addMenu(part));
            parent = groups.value(path);
        }
        if (labels[path].contains(profile.encoding))
            continue;
        labels[path].append(profile.encoding);
        action(parent, profile.encoding, profile.name);
    }
    menu->addSeparator();
    action(menu, "Raw", "Raw");
    gamut->setMenu(menu);
    connect(d.actions, &QActionGroup::triggered, this, [this](QAction* action) { select(action->data().toString()); });
    connect(encoding, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && d.encoding->isEnabled()) {
            d.encodingOverride = d.encoding->currentData().toString();
            d.encodingAutomatic = d.encodingOverride.isEmpty();
            refresh();
            Q_EMIT changed();
        }
    });
    select(initial, false);
}

void
ColorSelector::select(const QString& name, bool notify)
{
    const bool different = d.name != name || !d.encodingOverride.isEmpty();
    d.name = name;
    d.encodingOverride.clear();
    d.encodingAutomatic = name.isEmpty();
    refresh();
    if (notify && different)
        Q_EMIT changed();
}

void
ColorSelector::setAutoName(const QString& name)
{
    d.autoName = name;
    if (d.name.isEmpty())
        refresh();
}

QString
ColorSelector::name() const
{
    if (d.name == "Raw" || d.encodingOverride.isEmpty())
        return d.name;

    const QString base = d.name.isEmpty() ? d.autoName : d.name;
    QString gamut;
    for (const auto& profile : d.profiles)
        if (profile.name == base)
            gamut = profile.gamut;
    const QString encoding = d.encodingOverride == "Raw" ? "Linear" : d.encodingOverride;
    for (const auto& profile : d.profiles)
        if (profile.gamut == gamut && profile.encoding == encoding)
            return profile.name;

    return base;
}

void
ColorSelector::refresh()
{
    const QSignalBlocker blocker(d.encoding);
    const QString effective = d.name.isEmpty() ? d.autoName : d.name;
    sdk::render::ColorProfile current { effective, effective, "As configured", {} };
    for (const auto& profile : d.profiles)
        if (profile.name == effective) {
            current = profile;
            break;
        }
    d.encoding->clear();
    d.encoding->addItem(effective.isEmpty() ? QStringLiteral("Auto") : "Auto: " + current.encoding, QString());
    d.encoding->insertSeparator(d.encoding->count());
    for (const auto& profile : d.profiles) {
        if (profile.gamut == current.gamut && d.encoding->findData(profile.encoding) < 0)
            d.encoding->addItem(profile.encoding, profile.encoding);
    }
    d.encoding->insertSeparator(d.encoding->count());
    d.encoding->addItem("Raw", "Raw");
    d.encoding->setEnabled(d.name != "Raw");
    const QString selected = d.name == "Raw"                ? QStringLiteral("Raw")
                             : d.encodingOverride.isEmpty() ? (d.encodingAutomatic ? QString() : current.encoding)
                                                            : d.encodingOverride;
    d.encoding->setCurrentIndex(d.encoding->findData(selected));
    d.gamut->setText(d.name == "Raw"    ? "Raw"
                     : d.name.isEmpty() ? (d.autoName.isEmpty() ? "Auto" : "Auto: " + current.gamut)
                                        : current.gamut);
    d.gamut->setToolTip(d.name.isEmpty() ? "Auto: " + d.autoName : name());
    d.encoding->setToolTip(
        "Encoding / gamma. Auto uses the selected profile; Raw bypasses the curve (linear). Resolved: "
        + (d.name.isEmpty() ? d.autoName : name()));
    for (auto* action : d.actions->actions())
        action->setChecked(action->data().toString() == d.name);
    d.gamut->setFixedWidth(170);
    d.encoding->setFixedWidth(170);
    for (int i = 0; i < d.encoding->count(); ++i)
        d.encoding->setItemData(i, int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
}

}  // namespace flipman
