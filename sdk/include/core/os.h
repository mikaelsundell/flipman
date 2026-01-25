// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <core/container.h>

#include <QObject>
#include <QScopedPointer>

namespace core {
class OSPrivate;
class OS : public Container {
    Q_OBJECT
public:
    enum Power { PowerOff, Restart, Sleep };
    OS();
    virtual ~OS();
    void stayawake(bool awake);
    core::Error error() const override;
    void reset() override;

    static QString programpath();
    static QString applicationpath();
    static QString resourcepath(const QString& resource);

Q_SIGNALS:
    void power_changed(Power power);

private:
    QScopedPointer<OSPrivate> p;
};
}  // namespace core
