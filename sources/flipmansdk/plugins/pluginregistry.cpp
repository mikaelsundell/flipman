// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QMap>
#include <plugins/mediareader.h>
#include <plugins/mediawriter.h>
#include <plugins/pluginregistry.h>

#include <plugins/qt/qt.h>
#include <plugins/quicktime/quicktime.h>

#include <QDebug>

namespace flipman::sdk::plugins {
class PluginRegistryPrivate {
public:
    struct Data {
        core::Error error;
        QList<PluginHandler> plugins;
    };
    Data d;
};

PluginRegistry::PluginRegistry()
    : p(new PluginRegistryPrivate())
{
    register_plugin(QuicktimeReader::handler());
    register_plugin(QtWriter::handler());
}

PluginRegistry::~PluginRegistry() { reset(); }

bool
PluginRegistry::register_plugin(const PluginHandler& handler)
{
    p->d.plugins.append(handler);
}

core::Plugin*
PluginRegistry::get_plugin(std::type_index type, const QString& extension) const
{
    for (PluginHandler& handler : p->d.plugins) {
        if (handler.pluginfactory.type == type && handler.pluginfactory.extensions().contains(extension)) {
            return handler.pluginfactory.creator();
        }
    }
    return nullptr;
}

bool
PluginRegistry::has_extension(std::type_index type, const QString& extension) const
{
    for (const PluginHandler& handler : p->d.plugins) {
        if (handler.pluginfactory.type == type) {
            QList<QString> extensions = handler.pluginfactory.extensions();
            if (extensions.contains(extension, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}

QList<core::Plugin*>
PluginRegistry::get_plugins() const
{
    QList<core::Plugin*> result;
    for (const PluginHandler& handler : p->d.plugins) {
        result.append(handler.pluginfactory.creator());
    }
    return result;
}

PluginRegistry*
PluginRegistry::instance()
{
    static PluginRegistry registry;
    return &registry;
}

void
PluginRegistry::reset()
{
    PluginRegistry* registery = instance();
    registery->p->d.plugins.clear();
    registery->p->d.error.reset();
}
}  // namespace flipman::sdk::plugins
