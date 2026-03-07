// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/plugins/fx/fxreader.h>
#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/mediawriter.h>
#include <flipmansdk/plugins/oiio/oiiowriter.h>
#include <flipmansdk/plugins/pluginregistry.h>
#include <flipmansdk/plugins/qt/qtwriter.h>
#include <flipmansdk/plugins/quicktime/quicktimereader.h>
#include <flipmansdk/plugins/quicktime/quicktimewriter.h>

#include <QMap>

namespace flipman::sdk::plugins {
class PluginRegistryPrivate {
public:
    struct Data {
        core::Error error;
        QList<PluginHandler> plugins;
    };
    Data d;
};

PluginRegistry::PluginRegistry(QObject* parent)
    : QObject(parent)
    , p(new PluginRegistryPrivate())
{
    registerPlugin(FxReader::handler());
    registerPlugin(QuicktimeReader::handler());
    registerPlugin(OIIOWriter::handler());
    registerPlugin(QtWriter::handler());
}

PluginRegistry::~PluginRegistry() {}

bool
PluginRegistry::registerPlugin(const PluginHandler& handler)
{
    p->d.plugins.append(handler);
}

core::Plugin*
PluginRegistry::getPlugin(std::type_index type, const QString& extension) const
{
    for (PluginHandler& handler : p->d.plugins) {
        if (handler.pluginfactory.type == type && handler.pluginfactory.extensions().contains(extension)) {
            return handler.pluginfactory.creator();
        }
    }
    return nullptr;
}

bool
PluginRegistry::hasExtension(std::type_index type, const QString& extension) const
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
PluginRegistry::getPlugins() const
{
    QList<core::Plugin*> result;
    for (const PluginHandler& handler : p->d.plugins) {
        result.append(handler.pluginfactory.creator());
    }
    return result;
}

}  // namespace flipman::sdk::plugins
