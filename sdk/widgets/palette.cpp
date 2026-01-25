// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#include <QHash>
#include <QMutex>
#include <QPointer>
#include <widgets/palette.h>

#include <QDebug>

namespace widgets {
QScopedPointer<Palette, Palette::Deleter> Palette::pi;

class PalettePrivate {
public:
    PalettePrivate();
    ~PalettePrivate();
    void init();
    QColor role(const QString& role) const;
    QString rolestring(Palette::Role role) const;

public:
    QHash<QString, QColor> roles;
};

PalettePrivate::PalettePrivate() {}

PalettePrivate::~PalettePrivate() {}

void
PalettePrivate::init()
{
    roles.insert("background", QColor(25, 25, 25));
    roles.insert("foreground", QColor(240, 240, 240));
    roles.insert("viewport", QColor(25, 25, 25));
    roles.insert("accent", QColor(100, 150, 250));
    roles.insert("highlight", QColor(255, 200, 0));
    roles.insert("border", QColor(50, 50, 50));
    roles.insert("text", QColor(220, 220, 220));
}

QColor
PalettePrivate::role(const QString& role) const
{
    return roles.value(role.toLower(), Qt::black);
}

QString
PalettePrivate::rolestring(Palette::Role role) const
{
    switch (role) {
    case Palette::Background: return "background";
    case Palette::Foreground: return "foreground";
    case Palette::Viewport: return "viewport";
    case Palette::Accent: return "accent";
    case Palette::Highlight: return "highlight";
    case Palette::Border: return "border";
    case Palette::Text: return "text";
    }
    return QString();
}

Palette::Palette()
    : p(new PalettePrivate())
{
    p->init();
}

Palette::~Palette() {}

Palette*
Palette::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!pi) {
        pi.reset(new Palette());
    }
    return pi.data();
}

QColor
Palette::color(Role role) const
{
    return p->role(p->rolestring(role));
}

QColor
Palette::color(const QString& role) const
{
    return p->role(role);
}
}  // namespace widgets
