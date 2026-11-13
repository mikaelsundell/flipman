// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "colorselector.h"

#include <QMap>
#include <algorithm>

namespace flipman {

ColorSelector::ColorSelector(QPushButton* gamut, QComboBox* encoding, const QList<sdk::render::ColorProfile>& profiles,
                             const QString& initial)
    : m_gamut(gamut)
    , m_encoding(encoding)
    , m_profiles(profiles)
{
    auto* menu = new QMenu(gamut);
    m_actions = new QActionGroup(menu);
    m_actions->setExclusive(true);
    const auto action = [this](QMenu* parent, const QString& label, const QString& name) {
        auto* item = parent->addAction(label);
        item->setCheckable(true);
        item->setData(name);
        item->setToolTip(name);
        m_actions->addAction(item);
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
    connect(m_actions, &QActionGroup::triggered, this, [this](QAction* action) { select(action->data().toString()); });
    connect(encoding, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && m_encoding->isEnabled()) {
            m_encodingOverride = m_encoding->currentData().toString();
            m_encodingAutomatic = m_encodingOverride.isEmpty();
            refresh();
            if (changed)
                changed();
        }
    });
    select(initial, false);
}

void
ColorSelector::select(const QString& name, bool notify)
{
    const bool different = m_name != name || !m_encodingOverride.isEmpty();
    m_name = name;
    m_encodingOverride.clear();
    m_encodingAutomatic = name.isEmpty();
    refresh();
    if (notify && different && changed)
        changed();
}

void
ColorSelector::setAutoName(const QString& name)
{
    m_autoName = name;
    if (m_name.isEmpty())
        refresh();
}

QString
ColorSelector::name() const
{
    if (m_name == "Raw" || m_encodingOverride.isEmpty())
        return m_name;

    const QString base = m_name.isEmpty() ? m_autoName : m_name;
    QString gamut;
    for (const auto& profile : m_profiles)
        if (profile.name == base)
            gamut = profile.gamut;
    const QString encoding = m_encodingOverride == "Raw" ? "Linear" : m_encodingOverride;
    for (const auto& profile : m_profiles)
        if (profile.gamut == gamut && profile.encoding == encoding)
            return profile.name;

    return base;
}

void
ColorSelector::refresh()
{
    const QSignalBlocker blocker(m_encoding);
    const QString effective = m_name.isEmpty() ? m_autoName : m_name;
    sdk::render::ColorProfile current { effective, effective, "As configured", {} };
    for (const auto& profile : m_profiles)
        if (profile.name == effective) {
            current = profile;
            break;
        }
    m_encoding->clear();
    m_encoding->addItem(effective.isEmpty() ? QStringLiteral("Auto") : "Auto: " + current.encoding, QString());
    m_encoding->insertSeparator(m_encoding->count());
    for (const auto& profile : m_profiles) {
        if (profile.gamut == current.gamut && m_encoding->findData(profile.encoding) < 0)
            m_encoding->addItem(profile.encoding, profile.encoding);
    }
    m_encoding->insertSeparator(m_encoding->count());
    m_encoding->addItem("Raw", "Raw");
    m_encoding->setEnabled(m_name != "Raw");
    const QString selected = m_name == "Raw"                ? QStringLiteral("Raw")
                             : m_encodingOverride.isEmpty() ? (m_encodingAutomatic ? QString() : current.encoding)
                                                            : m_encodingOverride;
    m_encoding->setCurrentIndex(m_encoding->findData(selected));
    m_gamut->setText(m_name == "Raw"    ? "Raw"
                     : m_name.isEmpty() ? (m_autoName.isEmpty() ? "Auto" : "Auto: " + current.gamut)
                                        : current.gamut);
    m_gamut->setToolTip(m_name.isEmpty() ? "Auto: " + m_autoName : name());
    m_encoding->setToolTip(
        "Encoding / gamma. Auto uses the selected profile; Raw bypasses the curve (linear). Resolved: "
        + (m_name.isEmpty() ? m_autoName : name()));
    for (auto* action : m_actions->actions())
        action->setChecked(action->data().toString() == m_name);
    m_gamut->setFixedWidth(170);
    m_encoding->setFixedWidth(170);
    for (int i = 0; i < m_encoding->count(); ++i)
        m_encoding->setItemData(i, int(Qt::AlignLeft | Qt::AlignVCenter), Qt::TextAlignmentRole);
}

}  // namespace flipman
