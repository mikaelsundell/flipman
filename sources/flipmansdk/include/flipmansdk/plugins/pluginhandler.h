// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/plugin.h>

#include <functional>
#include <typeindex>

namespace flipman::sdk::plugins {

/**
 * @class PluginHandler
 * @brief Describes a plugin and provides instantiation logic.
 *
 * Stores metadata and factory functions used by PluginRegistry
 * to resolve and create plugin instances.
 */
class PluginHandler {
public:
    /**
     * @struct Info
     * @brief Metadata describing the plugin.
     */
    struct Info {
        QString name;         ///< Plugin name.
        QString description;  ///< Short description.
        QString version;      ///< Version string.
    };

    /**
     * @struct Factory
     * @brief Runtime factory configuration.
     *
     * Defines the plugin type, supported extensions,
     * and the creator function.
     */
    struct Factory {
        std::type_index type;                        ///< RTTI type index of the plugin class.
        std::function<QList<QString>()> extensions;  ///< Returns supported file extensions.
        std::function<core::Plugin*()> creator;      ///< Creates a new plugin instance.
    };

public:
    /**
     * @brief Constructs a PluginHandler.
     *
     * @param info Plugin metadata.
     * @param factory Factory configuration.
     */
    explicit PluginHandler(const Info& info, const Factory& factory)
        : plugininfo(info)
        , pluginfactory(factory)
    {}

    /**
     * @brief Creates a PluginHandler for type T.
     *
     * @tparam T Plugin interface type.
     * @param info Plugin metadata.
     * @param exts Function returning supported extensions.
     * @param create Function creating plugin instances.
     *
     * @return Configured PluginHandler.
     */
    template<typename T>
    static PluginHandler create(const Info& info, std::function<QList<QString>()> exts,
                                std::function<core::Plugin*()> create)
    {
        Factory factory = { typeid(T), std::move(exts), std::move(create) };
        return PluginHandler(info, factory);
    }

public:
    Info plugininfo;        ///< Plugin metadata.
    Factory pluginfactory;  ///< Factory configuration.
};

}  // namespace flipman::sdk::plugins

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::plugins::PluginHandler)
