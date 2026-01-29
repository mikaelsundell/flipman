// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/plugin.h>

#include <typeindex>

namespace flipman::sdk::plugins {
class PluginHandler {
public:
    struct Info {
        QString name;
        QString description;
        QString version;
    };
    struct Factory {
        std::type_index type;
        std::function<QList<QString>()> extensions;
        std::function<core::Plugin*()> creator;
    };
    PluginHandler(const Info& info, const Factory& factory)
        : plugininfo(info)
        , pluginfactory(factory)
    {}

    template<typename T>
    static PluginHandler create(const Info& info, std::function<QList<QString>()> exts,
                                std::function<core::Plugin*()> create)
    {
        Factory factory = { typeid(T), std::move(exts), std::move(create) };
        return PluginHandler(info, factory);
    }
    Info plugininfo;
    Factory pluginfactory;
    ;
};
}  // namespace flipman::sdk::plugins
