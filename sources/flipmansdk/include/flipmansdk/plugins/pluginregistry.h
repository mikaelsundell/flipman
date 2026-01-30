// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/plugin.h>
#include <flipmansdk/plugins/pluginhandler.h>

#include <QList>
#include <QScopedPointer>

namespace flipman::sdk::plugins {

class PluginRegistryPrivate;

/**
 * @class PluginRegistry
 * @brief The central manager for all registered SDK plugins.
 * * The PluginRegistry is a singleton that maintains a database of available plugins,
 * allowing the SDK to find the correct plugin for a specific file extension or
 * functional requirement.
 */
class FLIPMANSDK_EXPORT PluginRegistry {
public:
    /**
     * @brief Registers a new plugin via its handler.
     * @param handler The handler containing metadata and factory logic.
     * @return True if registration was successful.
     */
    bool registerPlugin(const PluginHandler& handler);

    /**
     * @brief Convenience template to retrieve and cast a plugin instance.
     * @tparam T The expected plugin type (e.g., ImageReader).
     * @param extension The file extension to find a plugin for.
     * @return A pointer to the instantiated plugin, or nullptr if not found.
     */
    template<typename T> T* getPlugin(const QString& extension) const
    {
        return dynamic_cast<T*>(getPlugin(typeid(T), extension));
    }

    /**
     * @brief Checks if any plugin of type T supports the given extension.
     * @tparam T The plugin category.
     * @param extension The file extension (e.g., "exr").
     * @return True if a match is found.
     */
    template<typename T> bool hasExtension(const QString& extension) const
    {
        return hasExtension(typeid(T), extension);
    }

    /**
     * @brief Returns a list of all currently registered plugin instances.
     */
    QList<core::Plugin*> getPlugins() const;

    /**
     * @brief Returns the global instance of the registry.
     */
    static PluginRegistry* instance();

    /**
     * @brief Destroys the global instance and cleans up all plugins.
     */
    static void reset();

    /**
     * @brief Returns the last error encountered by the registry.
     */
    static core::Error error();

private:
    /**
     * @brief Internal lookup for plugins based on type index and extension.
     */
    core::Plugin* getPlugin(std::type_index type, const QString& extension) const;

    /**
     * @brief Internal check for extension support.
     */
    bool hasExtension(std::type_index type, const QString& extension) const;

    /**
     * @brief Private constructor for singleton pattern.
     */
    PluginRegistry();

    /**
     * @brief Private destructor.
     * @note Required for PIMPL cleanup of PluginRegistryPrivate.
     */
    ~PluginRegistry();

    QScopedPointer<PluginRegistryPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::plugins

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::plugins::PluginRegistry)
