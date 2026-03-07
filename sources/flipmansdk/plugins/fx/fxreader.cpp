// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/plugins/fx/fxreader.h>

#include <flipmansdk/render/imageeffect.h>
#include <flipmansdk/render/shadercomposer.h>

namespace flipman::sdk::plugins {

class FxReaderPrivate : public QSharedData {
public:
    bool open(const core::File& file, const FxReader::Options& options);
    static plugins::PluginHandler::Info info();
    static core::Plugin* creator();
    static QList<QString> extensions();
    struct Data {
        render::ImageEffect imageEffect;
        core::Error error;
    };
    Data d;
};

bool
FxReaderPrivate::open(const core::File& file, const FxReader::Options& options)
{
    if (!file.exists()) {
        d.error = core::Error(info().name, "failed to open file");
        return false;
    }
    render::ShaderComposer shaderComposer;
    render::ShaderDefinition shaderDefinition = shaderComposer.fromFile(file);
    if (!shaderComposer.isValid()) {
        d.error = shaderComposer.error();
        return false;
    }
    d.imageEffect = render::ImageEffect();
    d.imageEffect.setShaderDefinition(shaderDefinition);
    return true;
}

plugins::PluginHandler::Info
FxReaderPrivate::info()
{
    return { "FxReader", "Reads effect files", "1.0.0" };
}

core::Plugin*
FxReaderPrivate::creator()
{
    return new FxReader();
}

QList<QString>
FxReaderPrivate::extensions()
{
    static const QList<QString> extensions = { "fx" };
    return extensions;
}

FxReader::FxReader(QObject* parent)
    : plugins::ImageEffectReader(parent)
    , p(new FxReaderPrivate())
{}

FxReader::~FxReader() {}

bool
FxReader::open(const core::File& file, const Options& options)
{
    return p->open(file, options);
}

bool
FxReader::close()
{
    return true;
}

bool
FxReader::isOpen() const
{
    return false;
}

render::ImageEffect
FxReader::imageEffect() const
{
    return p->d.imageEffect;
}

core::Error
FxReader::error() const
{
    return p->d.error;
}

plugins::PluginHandler
FxReader::handler()
{
    static plugins::PluginHandler handler
        = plugins::PluginHandler::create<ImageEffectReader>(FxReaderPrivate::info(), FxReaderPrivate::extensions,
                                                            FxReaderPrivate::creator);
    return handler;
}

}  // namespace flipman::sdk::plugins
