// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <QColor>
#include <QColorSpace>
#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::core {

class StylePrivate;

/**
 * @class Style
 * @brief Manages the visual appearance, themes, and branding of the SDK.
 * * Provides centralized control over colors, fonts, and color spaces. Changes
 * to style properties are broadcast via signals to ensure UI consistency.
 */
class FLIPMANSDK_EXPORT Style : public QObject {
    Q_OBJECT
public:
    /**
     * @enum ColorRole
     * @brief Semantic color definitions for UI elements.
     */
    enum ColorRole {
        Base,
        BaseAlt,
        Dock,
        DockAlt,
        Accent,
        AccentAlt,
        Text,
        TextDisabled,
        Highlight,
        HighlightAlt,
        Border,
        BorderAlt,
        Scrollbar,
        Progress,
        Button,
        ButtonAlt,
        Viewer,
        ViewerAlt
    };
    Q_ENUM(ColorRole)

    /**
     * @enum FontRole
     * @brief Logical font categories for consistent typography.
     */
    enum FontRole { DefaultSize, SmallSize, LargeSize };
    Q_ENUM(FontRole)

    /**
     * @enum Theme
     * @brief Global visual modes.
     */
    enum Theme { Dark, Light };
    Q_ENUM(Theme)

    /// Constructs the style manager and initializes default theme values.
    Style();

    virtual ~Style();

    /// Updates the current theme and emits themeChanged().
    void setTheme(Theme theme);

    /// Returns the currently active theme.
    Theme theme() const;

    /// Sets a specific color for a role and emits colorChanged().
    void setColor(ColorRole role, const QColor& color);

    /// Returns the color associated with the given role.
    QColor color(ColorRole role) const;

    /// Configures the active color space for rendering.
    void setColorSpace(const QColorSpace& colorSpace);

    /// Returns the current rendering color space.
    QColorSpace colorSpace() const;

    /// Sets the pixel size for a specific font role and emits fontChanged().
    void setFontSize(FontRole role, int size);

    /// Returns the pixel size for the given font role.
    int fontSize(FontRole role) const;

Q_SIGNALS:
    /// Emitted when the global theme is changed.
    void themeChanged(Theme theme);

    /// Emitted when a specific color role is modified.
    void colorChanged(ColorRole role);

    /// Emitted when a font size role is updated.
    void fontChanged(FontRole role);

private:
    Q_DISABLE_COPY_MOVE(Style)
    QScopedPointer<StylePrivate> p;
};

}  // namespace flipman::sdk::core
