// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QColor>
#include <QColorSpace>
#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::core {

class StylePrivate;

/**
 * @class Style
 * @brief Manages the visual appearance, themes, and branding of the SDK.
 *
 * Provides centralized control over colors, fonts, and color spaces. Changes
 * to style properties are broadcast via signals to ensure UI consistency
 * across all connected components.
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

    /**
     * @brief Constructs the style manager and initializes default theme values.
     */
    Style();

    /**
     * @brief Destroys the style.
     * @note Required for the PIMPL pattern to safely delete StylePrivate.
     */
    ~Style() override;

    /** @name Theme and Color Management */
    ///@{
    /**
     * @brief Updates the current theme and emits themeChanged().
     */
    void setTheme(Theme theme);

    /**
     * @brief Returns the currently active theme.
     */
    Theme theme() const;

    /**
     * @brief Sets a specific color for a role and emits colorChanged().
     */
    void setColor(ColorRole role, const QColor& color);

    /**
     * @brief Returns the color associated with the given role.
     */
    QColor color(ColorRole role) const;
    ///@}

    /** @name Rendering and Typography */
    ///@{
    /**
     * @brief Configures the active color space for rendering.
     */
    void setColorSpace(const QColorSpace& colorSpace);

    /**
     * @brief Returns the current rendering color space.
     */
    QColorSpace colorSpace() const;

    /**
     * @brief Sets the pixel size for a specific font role and emits fontChanged().
     */
    void setFontSize(FontRole role, int size);

    /**
     * @brief Returns the pixel size for the given font role.
     */
    int fontSize(FontRole role) const;
    ///@}

Q_SIGNALS:
    /**
     * @brief Emitted when the global theme is changed.
     */
    void themeChanged(Theme theme);

    /**
     * @brief Emitted when a specific color role is modified.
     */
    void colorChanged(ColorRole role);

    /**
     * @brief Emitted when a font size role is updated.
     */
    void fontChanged(FontRole role);

private:
    Q_DISABLE_COPY_MOVE(Style)
    QScopedPointer<StylePrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the style type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Style*)
