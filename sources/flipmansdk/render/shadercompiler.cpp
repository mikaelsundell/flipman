// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/shadercompiler.h>

#include <rhi/qshaderbaker.h>

namespace flipman::sdk::render {

class ShaderCompilerPrivate {
public:
    QShader compile(const QString& source, QShader::Stage stage, const ShaderCompiler::Options& options);
    struct Data {
        core::Error error;
    };
    Data d;
};


QShader
ShaderCompilerPrivate::compile(const QString& source, QShader::Stage stage, const ShaderCompiler::Options& options)
{
    d.error = core::Error();
    if (source.trimmed().isEmpty()) {
        d.error = core::Error("shadercompiler", "source is empty");
        return {};
    }
    QShaderBaker baker;
    baker.setSourceString(source.toUtf8(), stage);

    QList<QShaderBaker::GeneratedShader> targets;
    if (options.generateSpirv)
        targets.append({ QShader::SpirvShader, QShaderVersion(100) });

    if (options.generateHlsl)
        targets.append({ QShader::HlslShader, QShaderVersion(50) });

    if (options.generateMsl)
        targets.append({ QShader::MslShader, QShaderVersion(12) });

    targets.append({ QShader::GlslShader, QShaderVersion(options.glslVersion) });
    baker.setGeneratedShaders(targets);
    baker.setGeneratedShaderVariants({ QShader::StandardShader, QShader::BatchableVertexShader });

    QShaderBaker::SpirvOptions spirvOpts;
    if (options.optimize) {
        spirvOpts |= QShaderBaker::SpirvOption::StripDebugAndVarInfo;
    }

    baker.setSpirvOptions(spirvOpts);
    QShader shader = baker.bake();
    if (!shader.isValid()) {
        d.error = core::Error("shadercompiler", baker.errorMessage());
        return {};
    }
    return shader;
}

ShaderCompiler::ShaderCompiler(QObject* parent)
    : QObject(parent)
    , p(new ShaderCompilerPrivate())
{}

ShaderCompiler::~ShaderCompiler() = default;

QShader
ShaderCompiler::compile(const QString& source, QShader::Stage stage, const Options& options)
{
    return p->compile(source, stage, options);
}

core::Error
ShaderCompiler::error() const
{
    return p->d.error;
}

bool
ShaderCompiler::isValid() const
{
    return !p->d.error.hasError();
}

}  // namespace flipman::sdk::render
