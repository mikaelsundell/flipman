// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/environment.h>

#include <QCoreApplication>
#include <QDir>

namespace flipman::sdk::core {
class EnvironmentPrivate {
public:
    EnvironmentPrivate();
    ~EnvironmentPrivate();
    struct Data {};
    Data d;
};

EnvironmentPrivate::EnvironmentPrivate() {}

EnvironmentPrivate::~EnvironmentPrivate() {}

Environment::Environment()
    : p(new EnvironmentPrivate())
{}

Environment::~Environment() {}

QString
Environment::programPath()
{
    return QCoreApplication::applicationDirPath();
}

QString
Environment::applicationPath()
{
    QString path = programPath();
    QDir bundle(path);
    bundle.cdUp();
    bundle.cdUp();
    return bundle.absolutePath();
}

QString
Environment::resourcePath(const QString& resource)
{
    return QDir(applicationPath()).filePath(resource);
}
}  // namespace flipman::sdk::core
