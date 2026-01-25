// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QList>
#include <core/error.h>
#include <core/plugin.h>
#include <plugins/pluginhandler.h>

namespace plugins {
class PluginRegistryPrivate;
class PluginRegistry {
public:
    bool register_plugin(const PluginHandler& handler);
    template<typename T> T* get_plugin(const QString& extension) const
    {
        return dynamic_cast<T*>(get_plugin(typeid(T), extension));
    }
    template<typename T> bool has_extension(const QString& extension) const
    {
        return has_extension(typeid(T), extension);
    }
    QList<core::Plugin*> get_plugins() const;
    static PluginRegistry* instance();
    static void reset();
    static core::Error error();

private:
    core::Plugin* get_plugin(std::type_index type, const QString& extension) const;
    bool has_extension(std::type_index type, const QString& extension) const;
    PluginRegistry();
    ~PluginRegistry();
    QScopedPointer<PluginRegistryPrivate> p;
};
}  // namespace plugins
