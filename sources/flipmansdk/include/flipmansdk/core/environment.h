// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QMetaType>
#include <QScopedPointer>
#include <QString>

namespace flipman::sdk::core {

class EnvironmentPrivate;

/**
 * @class Environment
 * @brief Provides path resolution and environment context for the SDK.
 *
 * The Environment class handles the discovery of application binaries and
 * bundled resources, ensuring that the SDK can locate necessary assets
 * regardless of the installation directory or host OS.
 */
class FLIPMANSDK_EXPORT Environment {
public:
    /**
     * @brief Constructs the environment manager.
     */
    Environment();

    /**
     * @brief Destroys the environment manager.
     * @note Required for the PIMPL pattern to safely delete EnvironmentPrivate.
     */
    ~Environment();

    /**
     * @brief Returns the absolute path to the current executable.
     * @return A QString containing the full binary path.
     */
    static QString programPath();

    /**
     * @brief Returns the base directory where the application is located.
     * @return The directory path, often used to resolve relative configurations.
     */
    static QString applicationPath();

    /**
     * @brief Resolves the full path for a specific bundled resource.
     * @param resource The relative path or name of the resource (e.g., "icons/logo.png").
     * @return The absolute path to the resource on the local file system.
     */
    static QString resourcePath(const QString& resource);

private:
    Q_DISABLE_COPY_MOVE(Environment)
    QScopedPointer<EnvironmentPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Environment)
