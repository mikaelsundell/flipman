// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::core {

class SystemPrivate;

/**
 * @class System
 * @brief Manages host system interactions and power management.
 *
 * Provides an interface for controlling system behavior, such as preventing
 * display or system sleep during media playback or long-running tasks. This
 * class abstracts platform-specific OS calls into a unified SDK interface.
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

    /**
     * @brief Constructs the system manager.
     * @param parent The parent QObject for lifecycle management.
     */
    System(QObject* parent = nullptr);

    /**
     * @brief Destroys the system manager.
     * @note Required for the PIMPL pattern to safely delete SystemPrivate.
     */
    ~System() override;

    /** @name Power Inhibition */
    ///@{
    /**
     * @brief Sets whether the system should be prevented from entering sleep mode.
     * @param stayAwake If true, inhibits system/display sleep.
     */
    void setStayAwake(bool stayAwake);

    /**
     * @brief Returns true if the system is currently inhibited from sleeping.
     */
    bool isStayAwake() const;
    ///@}



Q_SIGNALS:
    /**
     * @brief Emitted when the system power state is about to change or has changed.
     */
    void powerStateChanged(PowerState powerState);

    /**
     * @brief Emitted when the stay-awake inhibition state changes.
     */
    void stayAwakeChanged(bool stayAwake);

private:
    Q_DISABLE_COPY_MOVE(System)
    QScopedPointer<SystemPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::System*)
