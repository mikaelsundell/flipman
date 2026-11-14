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

/**
 * @brief
 * UI-only adapter for grouped gamut and encoding choices. Owns no transforms
 * and performs no per-frame work. Its parent gamut widget owns Qt resources.
 */
class ColorSelector : public QObject {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates grouped gamut/encoding controls; widgets own their menus and are borrowed by this adapter.
     */
    ColorSelector(QPushButton* gamut, QComboBox* encoding, const QList<sdk::render::ColorProfile>& profiles,
                  const QString& initial);

    /**
     * @brief
     * Returns the selected OCIO encoding, empty for Auto or Raw for bypass.
     */
    QString name() const;

    /**
     * @brief
     * Updates both controls from a profile, optionally notifying observers of a user edit.
     */
    void select(const QString& name, bool notify = true);

    /**
     * @brief
     * Updates the profile resolved by Auto without changing an explicit user selection.
     */
    void setAutoName(const QString& name);

Q_SIGNALS:
    /**
     * @brief
     * Reports a user change to the selected gamut or encoding.
     */
    void changed();

private:
    /**
     * @brief
     * Refreshes control labels and enabled state without processing image pixels.
     */
    void refresh();

    struct Data {
        QList<sdk::render::ColorProfile> profiles;
        QString name;
        QString autoName;
        QString encodingOverride;
        QPushButton* gamut = nullptr;
        QComboBox* encoding = nullptr;
        QActionGroup* actions = nullptr;
        bool encodingAutomatic = false;
    };
    Data d;
};

}  // namespace flipman
