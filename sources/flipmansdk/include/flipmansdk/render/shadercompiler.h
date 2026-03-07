// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>

#include <QObject>
#include <QScopedPointer>
#include <rhi/qrhi.h>

namespace flipman::sdk::render {

class ShaderCompilerPrivate;

/**
 * @class ShaderCompiler
 * @brief Compiles GLSL shader source into portable QShader artifacts.
 *
 * ShaderCompiler translates high-level GLSL shader source into a serialized
 * QShader representation suitable for use with Qt's QRhi rendering system.
 *
 * Internally, the compiler orchestrates Qt's shader baking pipeline
 * to generate a multi-backend shader package.
 *
 */
class FLIPMANSDK_EXPORT ShaderCompiler : public QObject {
public:
    /**
     * @struct Options
     * @brief Controls compilation targets and optimization.
     */
    struct Options {
        /**
         * @brief Constructs default shader compilation options.
         */
        Options()
            : glslVersion(440)
            , generateSpirv(true)
            , generateMsl(true)
            , generateHlsl(true)
            , optimize(true)
        {}

        int glslVersion;     ///< GLSL version used as source input.
        bool generateSpirv;  ///< Enables SPIR-V generation (Vulkan/Metal).
        bool generateMsl;    ///< Enables Metal Shading Language output.
        bool generateHlsl;   ///< Enables HLSL output (Direct3D).
        bool optimize;       ///< Enables optimization passes.
    };

public:
    /**
     * @brief Constructs a ShaderCompiler.
     * @param parent Optional QObject parent.
     */
    explicit ShaderCompiler(QObject* parent = nullptr);

    /**
     * @brief Destroys the ShaderCompiler.
     *
     * Required for the PIMPL pattern to safely delete ShaderCompilerPrivate.
     */
    ~ShaderCompiler() override;

    /**
     * @brief Compiles GLSL source into a QShader.
     *
     * The provided source must represent a complete shader stage
     * (vertex, fragment, compute, etc.).
     *
     * @param source GLSL shader source.
     * @param stage  Shader stage (e.g. QShader::VertexStage).
     * @param options Compilation configuration.
     *
     * @return A serialized QShader containing the selected backend variants.
     *
     * @note If compilation fails, an invalid QShader is returned and
     *       error() will describe the failure.
     */
    QShader compile(const QString& source, QShader::Stage stage, const Options& options = Options());

    /** @name Status */
    ///@{

    /**
     * @brief Returns the last error encountered during compilation.
     */
    core::Error error() const;

    /**
     * @brief Returns true if the compiler is in a usable state.
     */
    bool isValid() const;

    ///@}

private:
    Q_DISABLE_COPY_MOVE(ShaderCompiler)
    QScopedPointer<ShaderCompilerPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::ShaderCompiler*)
