// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/render.h>
#include <QList>
#include <QString>
#include <QStringList>

namespace flipman::sdk::render {

/**
 * @brief OCIO GPU processing settings. Configure on the rendering thread between frames.
 *
 * Disabled by default for existing SDK consumers. The viewer enables management.
 * Empty configFile selects the embedded Studio v4 / ACES 2 / OCIO 2.5 config.
 * Effects and the existing layer blend use the selected working encoding. Un-tone-mapped preserves already rendered
 * video; select an ACES view explicitly for scene-linear or camera-log material.
 */
struct FLIPMANSDK_EXPORT ColorSettings {
    /**
     * @brief Enables GPU color management.
     */
    bool enabled = false;
    /**
     * @brief Optional external OCIO config filename; relative LUTs resolve beside it.
     */
    QString configFile;
    /**
     * @brief Effect working encoding, defaulting to ACEScg; camera log encodings are supported.
     */
    QString workingSpace = QStringLiteral("ACEScg");
    /**
     * @brief Output display or composable color-space identifier. Raw bypasses output processing.
     */
    QString display = QStringLiteral("Rec.1886 Rec.709 - Display");
    /**
     * @brief Display view applied once after compositing. Raw bypasses the complete output transform.
     */
    QString view = QStringLiteral("Un-tone-mapped");
    /**
     * @brief Compares all processing settings.
     */
    bool operator==(const ColorSettings& other) const;

    /**
     * @brief Returns whether any processing setting differs.
     */
    bool operator!=(const ColorSettings& other) const { return !(*this == other); }
};

/**
 * @brief Presentation metadata for an exact OCIO color-space identifier.
 *
 * Labels describe config profiles and composable gamut/curve pairs from the embedded config.
 */
struct FLIPMANSDK_EXPORT ColorProfile {
    /**
     * @brief Exact OCIO identifier used for processing and persistence.
     */
    QString name;
    /**
     * @brief Human-readable gamut label.
     */
    QString gamut;
    /**
     * @brief Human-readable encoding or gamma label.
     */
    QString encoding;
    /**
     * @brief OCIO family path used to group menus.
     */
    QString family;
};

/**
 * @brief OCIO config discovery and source interpretation; no CPU pixel processing.
 *
 * Functions throw std::exception-derived errors for invalid configs or names. Empty config
 * filenames use the embedded config, independent of a source checkout or OCIO env.
 */
class FLIPMANSDK_EXPORT ColorPipeline {
public:
    /**
     * @brief Resolves an independent RGB gamut/encoding pair to a cached OCIO identifier.
     *
     * The embedded config adds combinations of its linear RGB gamuts and known curves.
     * External configs offer only pairs explicitly defined by that config. Throws for unsupported pairs.
     */
    static QString combinedColorSpace(const QString& gamut, const QString& encoding, const QString& configFile = {});

    /**
     * @brief Lists named color spaces, including camera encodings and composable pairs.
     */
    static QStringList colorSpaces(const QString& configFile = {});

    /**
     * @brief Describes config color spaces as supported gamut/encoding pairs.
     *
     * Known Studio config names use explicit gamut/curve labels. Unknown
     * spaces retain their exact name and an "As configured" encoding label.
     */
    static QList<ColorProfile> colorProfiles(const QString& configFile = {});

    /**
     * @brief Returns the config family path used to group a named source encoding.
     */
    static QString colorSpaceFamily(const QString& name, const QString& configFile = {});

    /**
     * @brief Lists non-data scene-referred spaces available for effect processing.
     *
     * Includes scene-linear and encoded spaces such as ARRI LogC3 (EI800).
     * Blending in an encoded space is not linear-light compositing.
     */

    static QStringList workingSpaces(const QString& configFile = {});
    /**
     * @brief Lists output displays.
     */
    static QStringList displays(const QString& configFile = {});

    /**
     * @brief Lists views for an output display.
     */
    static QStringList views(const QString& display, const QString& configFile = {});

    /**
     * @brief Resolves supported image tags to an OCIO name; returns empty for unknown tags.
     *
     * Missing metadata requires an explicit layer inputColorSpace. No filename-based
     * camera gamut guesses are made.
     */
    static QString inputColorSpace(const core::ImageBuffer& image);

    /**
     * @brief Returns a matching existing Metal tag for known SDR display names.
     *
     * Returns false without modifying tag for outputs without an exact supported match.
     * The tag describes the display encoding, independently of the selected view.
     */
    static bool displayTag(const QString& display, DisplayTransform& tag);
};

}  // namespace flipman::sdk::render
