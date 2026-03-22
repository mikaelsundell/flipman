// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/render/shaderparser.h>

#include <flipmansdk/core/log.h>

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

namespace flipman::sdk::render {

using ParamType = ShaderDescriptor::ShaderParameter::Type;

class ShaderParserPrivate {
public:
    ParamType toParamType(const QString& type) const;
    QString replaceBlock(const QString& line, bool& block, bool& empty);
    QString replaceInclude(const QString& includeName, const QString& baseDir, QSet<QString>& includeStack);
    bool replaceInjections(QString& source, const ShaderParser::Injections& injections);
    bool parseParamLine(const QString& line, ShaderDescriptor::ShaderParameter& parameter);
    bool parseFunctionLine(const QString& line, ShaderFunction& function);
    bool parseSource(const QString& source, const QString& sourceDir, QString& shaderCode,
                     QVector<ShaderFunction>& functions, ShaderDescriptor& descriptor, QSet<QString>& includeStack);
    ShaderDefinition parse(const core::File& file, const ShaderParser::Options& options);
    ShaderDefinition parse(const QString& source, const QString& sourceDir, const ShaderParser::Options& options);
    struct Data {
        core::Error error;
    };
    Data d;
};

ParamType
ShaderParserPrivate::toParamType(const QString& t) const
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

bool
ShaderParserPrivate::parseParamLine(const QString& line, ShaderDescriptor::ShaderParameter& parameter)
{
    QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.size() < 3) {
        d.error = core::Error("shaderparser", "invalid @param declaration: " + line);
        return false;
    }
    parameter.name = tokens[2];
    parameter.type = toParamType(tokens[1]);
    if (tokens.size() >= 4)
        parameter.defaultValue = tokens[3];
    if (tokens.size() >= 5)
        parameter.minValue = tokens[4];
    if (tokens.size() >= 6)
        parameter.maxValue = tokens[5];
    return true;
}

bool
ShaderParserPrivate::parseFunctionLine(const QString& line, ShaderFunction& function)
{
    static QRegularExpression re(R"(\b([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\))");
    auto match = re.match(line);
    if (!match.hasMatch())
        return false;
    function.returnType = match.captured(1);
    function.name = match.captured(2);
    QString params = match.captured(3).trimmed();
    if (!params.isEmpty()) {
        QStringList list = params.split(",", Qt::SkipEmptyParts);
        for (QString p : list) {
            p = p.trimmed();
            QStringList tokens = p.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (!tokens.isEmpty())
                function.parameterTypes.append(tokens.first());
        }
    }
    return true;
}

QString
ShaderParserPrivate::replaceInclude(const QString& includeName, const QString& baseDir, QSet<QString>& includeStack)
{
    if (includeStack.contains(includeName)) {
        d.error = core::Error("shaderparser", "circular include detected: " + includeName);
        return {};
    }

    includeStack.insert(includeName);
    QFile resourceFile(":/flipmansdk/glsl/" + includeName);
    if (resourceFile.exists()) {
        if (!resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            d.error = core::Error("shaderparser", "failed to open built-in include: " + includeName);
            return {};
        }
        return QString::fromUtf8(resourceFile.readAll());
    }
    if (!baseDir.isEmpty()) {
        QFile localFile(baseDir + "/" + includeName);
        if (localFile.exists()) {
            if (!localFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                d.error = core::Error("shaderparser", "failed to open local include: " + includeName);
                return {};
            }
            return QString::fromUtf8(localFile.readAll());
        }
    }
    d.error = core::Error("shaderparser", "include not found: " + includeName);
    return {};
}


QString
ShaderParserPrivate::replaceBlock(const QString& line, bool& block, bool& empty)
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
ShaderParserPrivate::parseSource(const QString& source, const QString& sourceDir, QString& shaderCode,
                                 QVector<ShaderFunction>& functions, ShaderDescriptor& descriptor,
                                 QSet<QString>& includeStack)
{
    QTextStream output(&shaderCode);
    bool block = false;
    const QStringList lines = source.split('\n');

    for (const QString& originalLine : lines) {
        bool empty = false;
        QString line = replaceBlock(originalLine, block, empty);
        if (empty)
            continue;
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            if (trimmed.startsWith("@param")) {
                ShaderDescriptor::ShaderParameter param;
                if (!parseParamLine(trimmed, param))
                    return false;
                if (descriptor.indexOf(param.name) != -1) {
                    d.error = core::Error("shaderparser", "duplicate @param: " + param.name);
                    return false;
                }
                descriptor.parameters.append(param);
                continue;
            }
            if (trimmed.startsWith("@include")) {
                QStringList tokens = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (tokens.size() != 2) {
                    d.error = core::Error("shaderparser", "invalid @include syntax: " + line);
                    return false;
                }
                QString includeName = tokens[1];
                includeName.remove('"');

                QString includedCode = replaceInclude(includeName, sourceDir, includeStack);
                if (d.error.hasError())
                    return false;
                QString nestedShaderCode;
                if (!parseSource(includedCode, QFileInfo(sourceDir + "/" + includeName).absolutePath(),
                                 nestedShaderCode, functions, descriptor, includeStack))
                    return false;
                output << nestedShaderCode;
                continue;
            }
            ShaderFunction function;
            if (parseFunctionLine(trimmed, function)) {
                functions.append(function);
            }
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

bool
ShaderParserPrivate::replaceInjections(QString& source, const ShaderParser::Injections& injections)
{
    for (auto it = injections.tokens.begin(); it != injections.tokens.end(); ++it) {
        source.replace("@" + it.key(), it.value());
    }
    return true;
}

ShaderDefinition
ShaderParserPrivate::parse(const core::File& file, const ShaderParser::Options& options)
{
    QFile filePath(file.filePath());
    if (!filePath.open(QIODevice::ReadOnly | QIODevice::Text)) {
        d.error = core::Error("shaderparser", "failed to open shader file: " + file.filePath());
        return {};
    }
    QByteArray bytes = filePath.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    QString source = decoder.decode(bytes);
    if (decoder.hasError()) {
        d.error = core::Error("shaderparser", "shader file is not valid utf-8: " + file.filePath());
        return {};
    }
    filePath.close();
    QFileInfo info(filePath);
    QString sourceDir = info.absolutePath();
    return parse(source, sourceDir, options);
}

ShaderDefinition
ShaderParserPrivate::parse(const QString& source, const QString& sourceDir, const ShaderParser::Options& options)
{
    d.error = core::Error();
    QString shaderCode;
    ShaderDescriptor descriptor;
    QVector<ShaderFunction> functions;
    QSet<QString> includeStack;

    if (!parseSource(source, sourceDir, shaderCode, functions, descriptor, includeStack))
        return {};

    if (!replaceInjections(shaderCode, options.injections)) {
        return {};
    }

    ShaderDefinition def;
    def.setDescriptor(descriptor);
    def.setFunctions(functions);
    def.setShaderCode(shaderCode);
    return def;
}

ShaderParser::ShaderParser(QObject* parent)
    : QObject(parent)
    , p(new ShaderParserPrivate())
{}

ShaderParser::~ShaderParser() = default;

ShaderDefinition
ShaderParser::parse(const core::File& file, const Options& options)
{
    return p->parse(file, options);
}

ShaderDefinition
ShaderParser::parse(const QString& source, const Options& options)
{
    return p->parse(source, QString(), options);
}

core::Error
ShaderParser::error() const
{
    return p->d.error;
}

bool
ShaderParser::isValid() const
{
    return !p->d.error.hasError();
}

}  // namespace flipman::sdk::render
