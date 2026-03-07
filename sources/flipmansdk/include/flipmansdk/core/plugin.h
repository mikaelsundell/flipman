// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>

#include <QObject>

namespace flipman::sdk::core {

/**
 * @class Plugin
 * @brief Base class for SDK plugins.
 */
class FLIPMANSDK_EXPORT Plugin : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a Plugin.
     *
     * @param parent Optional QObject parent.
     */
    explicit Plugin(QObject* parent = nullptr);

    /**
     * @brief Destroys the Plugin.
     */
    ~Plugin() override;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Plugin*)
