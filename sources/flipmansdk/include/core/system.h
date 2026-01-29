// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <QScopedPointer>

namespace flipman::sdk::core {

class SystemPrivate;

/**
 * @class System
 * @brief Manages host system interactions and power management.
 * * Provides an interface for controlling system behavior, such as preventing
 * display or system sleep during media playback or long-running tasks.
 */
class FLIPMANSDK_EXPORT System : public QObject {
    Q_OBJECT
public:
    /**
     * @enum PowerState
     * @brief Represents the various power-related states or transitions of the host system.
     */
    enum PowerState { PowerOff, Restart, Sleep };
    Q_ENUM(PowerState)

    /// Constructs the system manager.
    System();

    virtual ~System();

    /**
     * @brief Sets whether the system should be prevented from entering sleep mode.
     * @param stayAwake If true, inhibits system/display sleep.
     */
    void setStayAwake(bool stayAwake);

    /// Returns true if the system is currently inhibited from sleeping.
    bool isStayAwake() const;

Q_SIGNALS:
    /// Emitted when the system power state is about to change or has changed.
    void powerStateChanged(PowerState powerState);

    /// Emitted when the stay-awake inhibition state changes.
    void stayAwakeChanged(bool stayAwake);

private:
    Q_DISABLE_COPY_MOVE(System)
    QScopedPointer<SystemPrivate> p;
};

}  // namespace flipman::sdk::core
