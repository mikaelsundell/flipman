// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/shadercomposer.h>

#include <flipmansdk/core/log.h>

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

namespace flipman::sdk::render {

using ParamType = ShaderDefinition::ShaderParameter::Type;

class ShaderComposerPrivate {
public:
    ParamType parseType(const QString& type) const;
    QString glslType(ParamType type) const;
    bool parseParamLine(const QString& line, ShaderDefinition::ShaderParameter& out);
    QString resolveInclude(const QString& includeName, const QString& baseDir, QSet<QString>& includeStack);
    QString lexLine(const QString& line, bool& block, bool& empty);
    bool interpretSource(const QString& source, const QString& baseDir, QString& shaderCode,
                         ShaderDefinition::ShaderDescriptor& descriptor, QSet<QString>& includeStack);
    void applyInjections(QString& source, const ShaderComposer::Injections& injections);
    ShaderDefinition fromFile(const core::File& file, const ShaderComposer::Options& options);
    ShaderDefinition fromSource(const QString& source, const ShaderComposer::Options& options,
                                const QString& baseDir = QString());
    struct Data {
        core::Error error;
    };
    Data d;
};

ParamType
ShaderComposerPrivate::parseType(const QString& t) const
{
    if (t == "float")
        return ParamType::Float;
    if (t == "int")
        return ParamType::Int;
    if (t == "bool")
        return ParamType::Bool;
    if (t == "vec2")
        return ParamType::Vec2;
    if (t == "vec3")
        return ParamType::Vec3;
    if (t == "vec4")
        return ParamType::Vec4;

    return ParamType::Float;
}

QString
ShaderComposerPrivate::glslType(ParamType t) const
{
    switch (t) {
    case ParamType::Float: return "float";
    case ParamType::Int: return "int";
    case ParamType::Bool: return "bool";
    case ParamType::Vec2: return "vec2";
    case ParamType::Vec3: return "vec3";
    case ParamType::Vec4: return "vec4";
    }
    return "float";
}

bool
ShaderComposerPrivate::parseParamLine(const QString& line, ShaderDefinition::ShaderParameter& out)
{
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.size() < 3) {
        d.error = core::Error("shadecomposer", "invalid @param declaration: " + line);
        return false;
    }

    out.name = tokens[2];
    out.type = parseType(tokens[1]);

    if (tokens.size() >= 4)
        out.defaultValue = tokens[3];
    if (tokens.size() >= 5)
        out.minValue = tokens[4];
    if (tokens.size() >= 6)
        out.maxValue = tokens[5];

    return true;
}

QString
ShaderComposerPrivate::resolveInclude(const QString& includeName, const QString& baseDir, QSet<QString>& includeStack)
{
    if (includeStack.contains(includeName)) {
        d.error = core::Error("shaderinterpreter", "circular include detected: " + includeName);
        return {};
    }

    includeStack.insert(includeName);
    QFile resourceFile(":/flipmansdk/fx/" + includeName);
    if (resourceFile.exists()) {
        if (!resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            d.error = core::Error("shaderinterpreter", "failed to open built-in include: " + includeName);
            return {};
        }
        return QString::fromUtf8(resourceFile.readAll());
    }
    if (!baseDir.isEmpty()) {
        QFile localFile(baseDir + "/" + includeName);
        if (localFile.exists()) {
            if (!localFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                d.error = core::Error("shaderinterpreter", "failed to open local include: " + includeName);
                return {};
            }
            return QString::fromUtf8(localFile.readAll());
        }
    }
    d.error = core::Error("shaderinterpreter", "include not found: " + includeName);
    return {};
}


QString
ShaderComposerPrivate::lexLine(const QString& line, bool& block, bool& empty)
{
    QString result;
    bool inString = false;
    bool escape = false;
    for (qsizetype i = 0; i < line.size(); ++i) {
        QChar c = line[i];
        QChar next = (i + 1 < line.size()) ? line[i + 1] : QChar();
        if (block) {
            if (c == '*' && next == '/') {
                block = false;
                ++i;
            }
            continue;
        }
        if (inString) {
            result += c;
            if (escape) {
                escape = false;
            }
            else if (c == '\\') {
                escape = true;
            }
            else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            inString = true;
            result += c;
            continue;
        }
        if (c == '/' && next == '/') {
            break;
        }
        if (c == '/' && next == '*') {
            block = true;
            ++i;
            continue;
        }
        result += c;
    }
    empty = result.trimmed().isEmpty() && !line.trimmed().isEmpty();
    return result;
}

bool
ShaderComposerPrivate::interpretSource(const QString& source, const QString& baseDir, QString& shaderCode,
                                       ShaderDefinition::ShaderDescriptor& descriptor, QSet<QString>& includeStack)
{
    QTextStream output(&shaderCode);
    bool block = false;
    const QStringList lines = source.split('\n');

    for (const QString& originalLine : lines) {
        bool empty = false;
        QString line = lexLine(originalLine, block, empty);
        if (empty)
            continue;

        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() && trimmed.startsWith("@param")) {
            ShaderDefinition::ShaderParameter param;
            if (!parseParamLine(trimmed, param))
                return false;

            if (descriptor.indexOf(param.name) != -1) {
                d.error = core::Error("shaderinterpreter", "duplicate @param: " + param.name);
                return false;
            }
            descriptor.parameters.append(param);
            continue;
        }
        if (!trimmed.isEmpty() && trimmed.startsWith("@include")) {
            QStringList tokens = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            if (tokens.size() != 2) {
                d.error = core::Error("shaderinterpreter", "invalid @include syntax: " + line);
                return false;
            }

            QString includeName = tokens[1];
            includeName.remove('"');

            QString includedCode = resolveInclude(includeName, baseDir, includeStack);
            if (d.error.hasError())
                return false;

            QString nestedShaderCode;
            if (!interpretSource(includedCode, QFileInfo(baseDir + "/" + includeName).absolutePath(), nestedShaderCode,
                                 descriptor, includeStack))
                return false;

            output << nestedShaderCode;
            continue;
        }
        output << line << '\n';
    }

    int newline = 0;
    while (newline < shaderCode.size() && shaderCode[newline] == '\n') {
        ++newline;
    }
    shaderCode.remove(0, newline);

    qsizetype end = shaderCode.size();
    while (end > 0 && shaderCode[end - 1] == '\n')
        --end;
    shaderCode.truncate(end);
    shaderCode += '\n';
    return true;
}

void
ShaderComposerPrivate::applyInjections(QString& source, const ShaderComposer::Injections& injections)
{
    if (injections.tokens.isEmpty())
        return;

    for (auto it = injections.tokens.begin(); it != injections.tokens.end(); ++it) {
        source.replace("@" + it.key(), it.value());
    }
}

ShaderDefinition
ShaderComposerPrivate::fromFile(const core::File& file, const ShaderComposer::Options& options)
{
    QFile filePath(file.filePath());
    if (!filePath.open(QIODevice::ReadOnly | QIODevice::Text)) {
        d.error = core::Error("shaderinterpreter", "failed to open shader file: " + file.filePath());
        return {};
    }
    QString source = QString::fromUtf8(filePath.readAll());
    filePath.close();

    QFileInfo info(filePath);
    QString baseDir = info.absolutePath();
    return fromSource(source, options, baseDir);
}

ShaderDefinition
ShaderComposerPrivate::fromSource(const QString& source, const ShaderComposer::Options& options, const QString& baseDir)
{
    d.error = core::Error();
    QString shaderCode;
    ShaderDefinition::ShaderDescriptor descriptor;
    QSet<QString> includeStack;

    if (!interpretSource(source, baseDir, shaderCode, descriptor, includeStack))
        return {};

    applyInjections(shaderCode, options.injections);

    QString uniformBlock;
    if (!descriptor.parameters.isEmpty()) {
        QTextStream stream(&uniformBlock);
        stream << "layout(std140, binding = 2) uniform EffectParams\n{\n";
        for (const auto& param : descriptor.parameters)
            stream << "    " << glslType(param.type) << " " << param.name << ";\n";
        stream << "};\n";
    }
    ShaderDefinition def;
    def.setShaderCode(shaderCode);
    def.setUniformBlock(uniformBlock);
    def.setApplyCode("color = effect(color, vUV);");
    def.setDescriptor(descriptor);
    return def;
}

ShaderComposer::ShaderComposer(QObject* parent)
    : QObject(parent)
    , p(new ShaderComposerPrivate())
{}

ShaderComposer::~ShaderComposer() = default;

ShaderDefinition
ShaderComposer::fromFile(const core::File& file, const Options& options)
{
    return p->fromFile(file, options);
}

ShaderDefinition
ShaderComposer::fromSource(const QString& source, const Options& options)
{
    return p->fromSource(source, options);
}

core::Error
ShaderComposer::error() const
{
    return p->d.error;
}

bool
ShaderComposer::isValid() const
{
    return !p->d.error.hasError();
}

}  // namespace flipman::sdk::render
