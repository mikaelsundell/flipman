// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>

#include <QObject>

namespace flipman::sdk::core {

/**
 * @class Plugin
 * @brief Base class for all flipman SDK plugins.
 *
 * The Plugin class provides a standardized interface for extending flipman
 * functionality, such as custom readers, writers, or filters. It integrates
 * with the Qt Meta-Object system to allow for dynamic loading and parent-child
 * memory management.
 * * @ingroup core
 */
class FLIPMANSDK_EXPORT Plugin : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a new Plugin object.
     * @param parent The parent QObject for ownership and lifecycle management.
     */
    Plugin(QObject* parent = nullptr);

    /**
     * @brief Destroys the Plugin object.
     */
    virtual ~Plugin();

    /**
     * @brief Returns the last recorded error state of the plugin.
     * * This method allows users to query if a plugin operation (like initialization
     * or execution) failed. The Error object contains specific details regarding
     * the failure type and message.
     * * @return Error The current error state.
     */
    Error error() const;
};

}  // namespace flipman::sdk::core
