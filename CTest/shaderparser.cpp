// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include <QCoreApplication>
#include <QDebug>
#include <flipmansdk/render/shadercontract.h>
#include <flipmansdk/render/shaderparser.h>
#include <stdexcept>

using namespace flipman::sdk::render;

static void
require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int
main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        ShaderParser parser;
        const auto definition = parser.parse(R"GLSL(
// vec4 effect(vec4 fake, ivec2 pixel, ivec2 size) {}
vec4 effect(vec4 color, ivec2 pixel, ivec2 size);
vec4 helper(vec4 color) { return color; }
highp vec4 /* comment splits tokens */ effect
(
    const in highp vec4/**/color,
    in ivec2 pixel,
    ivec2 size
)
{
    if (pixel.x > 0) { return helper(color); }
    return color;
}
void main(void) { vec4 result = effect(vec4(1.0), ivec2(0), ivec2(1)); }
)GLSL");
        require(parser.isValid(), "multiline shader failed to parse");
        require(definition.functions().size() == 3, "calls/comments/prototypes counted as definitions");
        const auto functions = definition.functions();
        const auto& effect = functions[1];
        require(effect.matches("vec4", "effect", { "vec4", "ivec2", "ivec2" }), "incorrect AST parameter types");
        require(definition.functions()[2].parameterTypes.isEmpty(), "void should mean no parameters");
        ShaderContract contract;
        require(contract.validate(ShaderContract::Type::Effect, definition.functions()), "effect contract failed");

        const auto overloads = parser.parse("vec3 effect(vec3 c) { return c; }\n"
                                            "vec4 effect(vec4 c, ivec2 p, ivec2 s) { return c; }");
        require(parser.isValid() && contract.validate(ShaderContract::Type::Effect, overloads.functions()),
                "valid entry overload hidden by a different overload");
        require(!contract.error().hasError(), "overload left a stale contract error");

        const auto prototype = parser.parse("vec4 effect(vec4, ivec2, ivec2);");
        require(parser.isValid() && prototype.functions().isEmpty(), "prototype accepted as a definition");
        require(!contract.validate(ShaderContract::Type::Effect, prototype.functions()),
                "prototype satisfies contract");

        ShaderParser::Options options;
        options.injections.set("body", "vec4 effect(vec4 c, ivec2 p, ivec2 s) { return c; }");
        const auto injected = parser.parse("@body", options);
        require(parser.isValid() && injected.functions().size() == 1, "injected functions were not reflected");

        const auto arrays = parser.parse("vec4 effect(vec4 c[2][3], ivec2 p, ivec2 s) { return c[0]; }");
        require(parser.isValid(), "array parameter failed to parse");
        require(!contract.validate(ShaderContract::Type::Effect, arrays.functions()), "array matched vector contract");

        for (const auto* invalid :
             { "vec4 effect(vec4 c) { return c;", "vec4 effect(vec4 c) { return c]; }",
               "vec4 effect(unknown c) { return c; }", "vec4 effect(vec4 c ivec2 p) { return c; }",
               "vec4 effect(vec4 c,) { return c; }", "vec4 effect(vec4 c) { return c; } /* unterminated",
               "#define TYPE vec4\nTYPE effect(TYPE c) { return c; }", "#version 2147483648\nvoid main() {}",
               "#version 9999999999999999999999999\nvoid main() {}" }) {
            parser.parse(QString::fromUtf8(invalid));
            require(!parser.isValid() && parser.error().hasError(), "invalid or unsupported source silently accepted");
        }
        parser.parse("vec4 effect(vec4 c) { return c; }");
        require(parser.isValid(), "parse error did not reset");
        parser.parse(QString("vec4 effect() {}") + QChar(0));
        require(!parser.isValid(), "embedded NUL accepted");
        return 0;
    } catch (const std::exception& error) {
        qCritical() << error.what();
        return 1;
    }
}
