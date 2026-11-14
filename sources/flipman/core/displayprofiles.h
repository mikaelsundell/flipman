// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/render/render.h>
#include <QList>
#include <QString>

namespace flipman {

/**
 * @brief
 * Named surface tag; describes output pixels without converting them.
 */
struct DisplayProfile {
    /**
     * @brief
     * User-visible profile name shared by all application interfaces.
     */
    QString name;
    /**
     * @brief
     * Descriptor resolved through the SDK DisplayColorSpace facade.
     */
    sdk::render::DisplayTransform transform;
};

/**
 * @brief
 * Returns the supported native display tags in stable UI order.
 */
QList<DisplayProfile>
displayProfiles();

/**
 * @brief
 * Resolves an exact profile name, leaving the output unchanged when unknown.
 */
bool
findDisplayProfile(const QString& name, sdk::render::DisplayTransform& transform);

}  // namespace flipman
