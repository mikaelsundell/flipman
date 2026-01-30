// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/core/plugin.h>

#include <functional>
#include <typeindex>

namespace flipman::sdk::plugins {

/**
 * @class PluginHandler
 * @brief Manages the registration and instantiation of SDK plugins.
 * * The PluginHandler stores metadata (Info) and the functional logic (Factory)
 * required to identify and create instances of a specific plugin type at runtime.
 */
class PluginHandler {
public:
    /**
     * @struct Info
     * @brief Human-readable metadata for the plugin.
     */
    struct Info {
        QString name;         ///< Name of the plugin (e.g., "OpenEXR Reader").
        QString description;  ///< Brief overview of the plugin's purpose.
        QString version;      ///< Semantic versioning string (e.g., "1.0.0").
    };

    /**
     * @struct Factory
     * @brief Technical specifications for plugin creation.
     */
    struct Factory {
        std::type_index type;                        ///< The RTTI type index of the plugin class.
        std::function<QList<QString>()> extensions;  ///< Returns supported file extensions (if applicable).
        std::function<core::Plugin*()> creator;      ///< Lambda or function to instantiate the plugin.
    };

    /**
     * @brief Constructs a plugin handler with the given metadata and factory.
     */
    explicit PluginHandler(const Info& info, const Factory& factory)
        : plugininfo(info)
        , pluginfactory(factory)
    {}

    /**
     * @brief Helper template to create a handler for a specific plugin type T.
     * @param info Metadata for the plugin.
     * @param exts Function returning supported file extensions.
     * @param create Function returning a new instance of the plugin.
     * @return A configured PluginHandler instance.
     */
    template<typename T>
    static PluginHandler create(const Info& info, std::function<QList<QString>()> exts,
                                std::function<core::Plugin*()> create)
    {
        Factory factory = { typeid(T), std::move(exts), std::move(create) };
        return PluginHandler(info, factory);
    }

    Info plugininfo;        ///< The metadata associated with this handler.
    Factory pluginfactory;  ///< The factory logic associated with this handler.
};

}  // namespace flipman::sdk::plugins

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::plugins::PluginHandler)
