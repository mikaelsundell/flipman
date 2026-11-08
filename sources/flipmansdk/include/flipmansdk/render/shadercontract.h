// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QScopedPointer>
#include <QString>
#include <QVector>
#include <flipmansdk/core/error.h>
#include <flipmansdk/flipmansdk.h>
#include <flipmansdk/render/shaderfunction.h>

namespace flipman::sdk::render {

class ShaderContractPrivate;

/**
 * @class ShaderContract
 * @brief Validates shader entry functions against renderer contracts.
 */
class FLIPMANSDK_EXPORT ShaderContract {
public:
    /**
     * @brief Shader contract types supported by the renderer.
     */
    enum class Type {
        /**
         * @brief Image processing shader
         */
        Effect,

        /**
         * @brief Input device transform
         */
        Idt,

        /**
         * @brief Output device transform
         */
        Odt
    };

public:
    /**
     * @brief Constructs an empty ShaderContract.
     *
     * Initializes a shader contract helper used for validating
     * shader functions against the supported renderer contracts.
     */
    ShaderContract();

    /**
     * @brief Destroys the ShaderContract.
     */
    ~ShaderContract();

    /**
     * @brief Returns display name for the shader type.
     */
    QString name(Type type);

    /**
     * @brief Returns required entry function signature.
     */
    QString signature(Type type);

    /**
     * @brief Returns invocation snippet used by the renderer.
     */
    QString call(Type type);

    /**
     * @brief Returns entry function name.
     */
    QString entryFunction(Type type);

    /**
     * @brief Validates parsed shader functions against the contract.
     *
     * @return True if valid.
     */
    bool validate(Type type, const QVector<ShaderFunction>& functions);

    /**
     * @name Status
     * @{
     */

    /**
     * @brief Returns the last contract validation error.
     */
    core::Error error() const;

    /**
     * @brief Returns true if no contract validation error is recorded.
     */
    bool isValid() const;

    /**
     * @}
     */

private:
    Q_DISABLE_COPY_MOVE(ShaderContract)
    /**
     * @brief Private implementation.
     */
    QScopedPointer<ShaderContractPrivate> p;
};

}  // namespace flipman::sdk::render

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::render::ShaderContract*)
