// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/render.h>
#include <QByteArray>
#include <QColorSpace>
#include <QMetaType>
#include <QString>
#include <QSurfaceFormat>

namespace flipman::sdk::render {

/**
 * @brief
 * Display-profile value used for Qt surfaces and native output tagging.
 *
 * Owns a QColorSpace and its cached ICC representation. Construction prepares
 * profile data; copies share Qt's value storage. This class describes pixels
 * and never transforms them. Native presentation handles belong to the viewer.
 */
class FLIPMANSDK_EXPORT DisplayColorSpace {
public:
    /**
     * @brief
     * Constructs an sRGB display profile without changing global state.
     */
    DisplayColorSpace();
    /**
     * @brief
     * Wraps a Qt profile and prepares its ICC representation.
     */
    explicit DisplayColorSpace(const QColorSpace& colorSpace);
    /**
     * @brief
     * Resolves an SDK display descriptor using the existing SDR mappings.
     *
     * Unsupported primaries or curves retain the viewer's documented fallback
     * behavior and emit a warning. OCIO working/input encodings are independent.
     */
    explicit DisplayColorSpace(const DisplayTransform& transform);
    /**
     * @brief
     * Imports an ICC profile; returns an invalid value if parsing fails.
     */
    static DisplayColorSpace fromIccProfile(const QByteArray& profile);
    /**
     * @brief
     * Returns whether the profile is valid.
     */
    bool isValid() const;
    /**
     * @brief
     * Returns the profile's descriptive name.
     */
    QString description() const;
    /**
     * @brief
     * Returns cached ICC data without generating a profile on each call.
     */
    QByteArray iccProfile() const;
    /**
     * @brief
     * Returns the Qt representation for explicit framework integration.
     */
    QColorSpace qtColorSpace() const;
    /**
     * @brief
     * Sets only the color-space field on the supplied format.
     *
     * Other format fields and the global default format are unaffected.
     */
    void applyTo(QSurfaceFormat& format) const;
    /**
     * @brief
     * Compares the represented display profiles.
     */
    bool operator==(const DisplayColorSpace& other) const;

    /**
     * @brief
     * Returns whether the represented display profiles differ.
     */
    bool operator!=(const DisplayColorSpace& other) const { return !(*this == other); }

private:
    QColorSpace m_colorSpace;
    QByteArray m_iccProfile;
};

}  // namespace flipman::sdk::render

/**
 * @note Registers display profiles for QVariant and Qt signals/slots.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::DisplayColorSpace)
