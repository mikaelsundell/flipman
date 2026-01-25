// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include <QColor>
#include <QScopedPointer>

#define palette_role(role) widgets::Palette::instance()->color(widgets::Palette::role)

namespace widgets {
class PalettePrivate;
class Palette {
public:
    enum Role { Background, Foreground, Viewport, Accent, Highlight, Border, Text };
    static Palette* instance();
    QColor color(Role role) const;
    QColor color(const QString& role) const;
    QString stylesheet();

private:
    Palette();
    ~Palette();
    Palette(const Palette&) = delete;
    Palette& operator=(const Palette&) = delete;
    class Deleter {
    public:
        static void cleanup(Palette* pointer) { delete pointer; }
    };
    static QScopedPointer<Palette, Deleter> pi;
    QScopedPointer<PalettePrivate> p;
};
}  // namespace widgets
