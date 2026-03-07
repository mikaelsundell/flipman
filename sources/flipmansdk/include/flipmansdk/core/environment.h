// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QMetaType>
#include <QScopedPointer>
#include <QString>

namespace flipman::sdk::core {

class EnvironmentPrivate;

/**
 * @class Environment
 * @brief Provides path resolution utilities for the SDK.
 */
class FLIPMANSDK_EXPORT Environment {
public:
    /**
     * @brief Constructs an Environment instance.
     */
    Environment();

    /**
     * @brief Destroys the Environment instance.
     */
    ~Environment();

    /**
     * @brief Returns the absolute path to the current executable.
     */
    static QString programPath();

    /**
     * @brief Returns the application directory path.
     */
    static QString applicationPath();

    /**
     * @brief Resolves a bundled resource path.
     *
     * @param resource Relative resource path.
     *
     * @return Absolute resource path.
     */
    static QString resourcePath(const QString& resource);

private:
    Q_DISABLE_COPY_MOVE(Environment)
    QScopedPointer<EnvironmentPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Environment)
