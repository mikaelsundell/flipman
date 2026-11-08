// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "glslreflection.h"
#include <cstdlib>
#include <cstring>
#include <parser.h>

// Upstream's lexeme table intentionally uses repeatedly defined macros.
#undef TYPE
#undef KEYWORD
#undef OPERATOR
#undef TYPENAME
#define TYPE(...)
#define KEYWORD(...)
#define OPERATOR(...)
#define TYPENAME(name) \
    case glsl::kKeyword_##name: return QStringLiteral(#name);
namespace {
QString
typeName(const glsl::astType* type)
{
    if (!type->builtin)
        return QString::fromUtf8(static_cast<const glsl::astStruct*>(type)->name);
    switch (static_cast<const glsl::astBuiltin*>(type)->type) {
#include <lexemes.h>
    default: return {};
    }
}

class ReflectionLexer : public glsl::lexer {
public:
    using glsl::lexer::lexer;
    ~ReflectionLexer() { current.clear(); }
    const glsl::token& nextToken()
    {
        read(current, true);
        return current;
    }
    size_t offset() const { return position(); }

private:
    glsl::token current = peek();
};
}  // namespace
#undef TYPE
#undef KEYWORD
#undef OPERATOR
#undef TYPENAME

namespace flipman::sdk::render {
bool
reflectGlslFunctions(const QString& source, QVector<ShaderFunction>& functions, QString& error)
{
    const QByteArray bytes = source.toUtf8();
    if (bytes.contains('\0')) {
        error = "GLSL source contains a NUL byte";
        return false;
    }
    ReflectionLexer lexer(bytes.constData());
    QVector<int> delimiters;
    qsizetype declarationStart = 0;
    int previousKind = glsl::kType_eof;
    bool functionCandidate = false;
    bool assignment = false;
    bool sawParenthesis = false;
    auto reset = [&] {
        declarationStart = qsizetype(lexer.offset());
        functionCandidate = assignment = sawParenthesis = false;
    };
    for (;;) {
        const qsizetype tokenStart = qsizetype(lexer.offset());
        const auto& token = lexer.nextToken();
        if (lexer.error()) {
            error = QString("GLSL line %1: %2").arg(lexer.line()).arg(QString::fromUtf8(lexer.error()));
            return false;
        }
        if (token.kind() == glsl::kType_eof)
            break;
        if (delimiters.isEmpty()) {
            if (token.kind() == glsl::kType_directive) {
                reset();
            }
            if (token.isOperator(glsl::kOperator_assign))
                assignment = true;
            if (token.isOperator(glsl::kOperator_paranthesis_begin) && !sawParenthesis) {
                functionCandidate = previousKind == glsl::kType_identifier && !assignment;
                sawParenthesis = true;
            }
            if (token.kind() == glsl::kType_scope_begin && functionCandidate) {
                // Parse a prototype AST from the complete, possibly multiline
                // declaration. Bodies may reference uniforms/helpers injected
                // later, so they belong to the final shader compiler.
                const QByteArray declaration = bytes.mid(declarationStart, tokenStart - declarationStart) + ';';
                glsl::parser parser(declaration.constData(), "flipman declaration");
                const auto* unit = parser.parse(glsl::astTU::kFragment);
                if (!unit || unit->functions.size() != 1) {
                    error = QString("GLSL declaration near line %1: %2")
                                .arg(lexer.line())
                                .arg(QString::fromUtf8(parser.error()));
                    return false;
                }
                const auto* parsed = unit->functions[0];
                ShaderFunction function(typeName(parsed->returnType), QString::fromUtf8(parsed->name));
                for (size_t i = 0; i < parsed->parameters.size(); ++i) {
                    const auto* parameter = parsed->parameters[i];
                    QString type = typeName(parameter->baseType);
                    // Arrays must never match a scalar/vector entry contract.
                    if (parameter->isArray)
                        type += "[]";
                    function.parameterTypes.append(type);
                }
                functions.append(function);
            }
            if (token.kind() == glsl::kType_semicolon)
                reset();
        }
        int opening = 0;
        int closing = 0;
        if (token.kind() == glsl::kType_scope_begin)
            opening = 1;
        if (token.kind() == glsl::kType_scope_end)
            closing = 1;
        if (token.isOperator(glsl::kOperator_paranthesis_begin))
            opening = 2;
        if (token.isOperator(glsl::kOperator_paranthesis_end))
            closing = 2;
        if (token.isOperator(glsl::kOperator_bracket_begin))
            opening = 3;
        if (token.isOperator(glsl::kOperator_bracket_end))
            closing = 3;
        if (opening)
            delimiters.append(opening);
        if (closing) {
            if (delimiters.isEmpty() || delimiters.takeLast() != closing) {
                error = QString("GLSL line %1: mismatched delimiter").arg(lexer.line());
                return false;
            }
            if (delimiters.isEmpty() && closing == 1)
                reset();
        }
        previousKind = token.kind();
    }
    if (!delimiters.isEmpty()) {
        error = "GLSL source has an unterminated delimiter";
        return false;
    }
    return true;
}
}  // namespace flipman::sdk::render
