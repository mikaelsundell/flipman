// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "settings.h"

#include <QDir>
#include <QSettings>

namespace flipman {

class SettingsPrivate {
public:
    struct Data {
        QScopedPointer<QSettings> storage;
        bool isolated = false;
    };
    Data d;
};

Settings::Settings(const QString& directory, QObject* parent)
    : QObject(parent)
    , p(new SettingsPrivate)
{
    p->d.isolated = !directory.isEmpty();
    if (p->d.isolated) {
        if (!QDir().mkpath(directory))
            qFatal("Could not create isolated Flipman settings directory");
        p->d.storage.reset(new QSettings(QDir(directory).filePath("flipman.ini"), QSettings::IniFormat));
        p->d.storage->setFallbacksEnabled(false);
    }
    else
        p->d.storage.reset(new QSettings);
}

Settings::~Settings() { sync(); }

QVariant
Settings::value(const QString& key, const QVariant& defaultValue) const
{
    return p->d.storage->value(key, defaultValue);
}

void
Settings::setValue(const QString& key, const QVariant& value)
{
    if (p->d.storage->value(key) == value)
        return;

    p->d.storage->setValue(key, value);
    Q_EMIT changed(key, value);
}

QString
Settings::filename() const
{
    return p->d.storage->fileName();
}

bool
Settings::isIsolated() const
{
    return p->d.isolated;
}

void
Settings::sync()
{
    p->d.storage->sync();
}

}  // namespace flipman
