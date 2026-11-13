// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/render/colorpipeline.h>
#include <QActionGroup>
#include <QComboBox>
#include <QMenu>
#include <QObject>
#include <QPushButton>
#include <QSignalBlocker>
#include <functional>

namespace flipman {

// UI-only adapter. Owns no transforms and does no per-frame work.
class ColorSelector : public QObject {
public:
    ColorSelector(QPushButton* gamut, QComboBox* encoding, const QList<sdk::render::ColorProfile>& profiles,
                  const QString& initial);
    QString name() const;
    void select(const QString& name, bool notify = true);
    void setAutoName(const QString& name);
    std::function<void()> changed;

private:
    void refresh();
    QPushButton* m_gamut;
    QComboBox* m_encoding;
    QList<sdk::render::ColorProfile> m_profiles;
    QActionGroup* m_actions;
    QString m_name;
    QString m_autoName;
    QString m_encodingOverride;
    bool m_encodingAutomatic = false;
};

}  // namespace flipman
