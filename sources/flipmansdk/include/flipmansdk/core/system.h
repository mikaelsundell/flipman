// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::core {

class SystemPrivate;

/**
 * @class System
 * @brief Host system control interface.
 */
class FLIPMANSDK_EXPORT System : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Power state transitions.
     */
    enum PowerState { PowerOff, Restart, Sleep };
    Q_ENUM(PowerState)

    /**
     * @brief Constructs a System instance.
     */
    System(QObject* parent = nullptr);

    /**
     * @brief Destroys the System instance.
     */
    ~System() override;

    /** @name Power Control */
    ///@{

    /**
     * @brief Enables or disables sleep inhibition.
     */
    void setStayAwake(bool stayAwake);

    /**
     * @brief Returns true if sleep inhibition is active.
     */
    bool isStayAwake() const;

    ///@}

Q_SIGNALS:

    /**
     * @brief Emitted when the power state changes.
     */
    void powerStateChanged(PowerState powerState);

    /**
     * @brief Emitted when sleep inhibition changes.
     */
    void stayAwakeChanged(bool stayAwake);

private:
    Q_DISABLE_COPY_MOVE(System)
    QScopedPointer<SystemPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::System*)
