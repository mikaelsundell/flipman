// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#pragma once

#include <flipmansdk.h>

#include <QMetaType>
#include <QScopedPointer>
#include <QString>

namespace flipman::sdk::core {

class EnvironmentPrivate;

/**
 * @class Environment
 * @brief Provides path resolution and environment context for the SDK.
 * * The Environment class handles the discovery of application binaries and
 * bundled resources, ensuring that the SDK can locate necessary assets
 * regardless of the installation directory or host OS.
 */
class FLIPMANSDK_EXPORT Environment {
public:
    /// Constructs the environment manager.
    explicit Environment();

    virtual ~Environment();

    /**
     * @brief Returns the absolute path to the current executable.
     * @return A QString containing the full binary path.
     */
    QString programPath() const;

    /**
     * @brief Returns the base directory where the application is located.
     * @return The directory path, often used to resolve relative configurations.
     */
    QString applicationPath() const;

    /**
     * @brief Resolves the full path for a specific bundled resource.
     * @param resource The relative path or name of the resource (e.g., "icons/logo.png").
     * @return The absolute path to the resource on the local file system.
     */
    QString resourcePath(const QString& resource) const;

private:
    Q_DISABLE_COPY_MOVE(Environment)
    QScopedPointer<EnvironmentPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Environment)
