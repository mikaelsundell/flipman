// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/render/shaderdefinition.h>

#include <QScopedPointer>
#include <QString>
#include <QStringList>

namespace flipman::sdk::render {

class ShaderParserPrivate;

/**
 * @class ShaderParser
 * @brief Parses shader source into a ShaderDefinition.
 *
 * Interprets custom directives such as @param and @include
 * and produces a backend-agnostic ShaderDefinition.
 */
class FLIPMANSDK_EXPORT ShaderParser : public QObject {
public:
    /**
     * @struct Injection
     * @brief Code fragments injected into a shader template.
     *
     * Tokens map to template placeholders (e.g. @idt, @odt, @effect_code).
     */
    struct Injections {
        QHash<QString, QString> tokens;
        void set(const QString& name, const QString& code) { tokens.insert(name, code); }
        QString value(const QString& name) const { return tokens.value(name); }
        bool contains(const QString& name) const { return tokens.contains(name); }
    };

    /**
     * @struct Options
     * @brief Controls include resolution behavior.
     */
    struct Options {
        /**
         * @brief Constructs Options with default values.
         */
        Options()
            : allowRelativeIncludes(true)
            , descriptorBinding(0)
        {}

        QStringList includeSearchPaths;  ///< Additional include search paths.
        bool allowRelativeIncludes;      ///< Allows includes relative to source file.
        int descriptorBinding;           ///< Binding index for the generated params uniform block.
        Injections injections;           ///< Optional template injection
    };

public:
    /**
     * @brief Constructs a ShaderParser.
     * @param parent Optional QObject parent.
     */
    explicit ShaderParser(QObject* parent = nullptr);

    /**
     * @brief Destroys the ShaderParser.
     */
    ~ShaderParser();

    /**
     * @brief Parse shader source from file.
     *
     * @param filePath Path to the shader file.
     * @param options  Interpretation options.
     * @return A parsed ShaderDefinition.
     */
    ShaderDefinition parse(const core::File& file, const Options& options = Options());

    /**
     * @brief Parse raw image effect shader source.
     *
     * @param source  Raw shader source code.
     * @param options Interpretation options.
     * @return A parsed ShaderDefinition.
     */
    ShaderDefinition parse(const QString& source, const Options& options = Options());

    /** @name Status */
    ///@{

    /**
     * @brief Returns the last interpreter error.
     */
    core::Error error() const;

    /**
     * @brief Returns true if the interpreter is usable.
     */
    bool isValid() const;

    ///@}

private:
    Q_DISABLE_COPY_MOVE(ShaderParser)
    QScopedPointer<ShaderParserPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::ShaderParser*)
