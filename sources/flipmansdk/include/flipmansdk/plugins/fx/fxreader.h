// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/plugins/imageeffectreader.h>
#include <flipmansdk/plugins/pluginhandler.h>
#include <flipmansdk/render/imageeffect.h>

#include <QScopedPointer>

namespace flipman::sdk::plugins {

class FxReaderPrivate;

/**
 * @class FxReader
 * @brief ImageEffectReader implementation for .fx-based effect files.
 *
 * Loads ImageEffect definitions from disk.
 */
class FLIPMANSDK_EXPORT FxReader : public ImageEffectReader {
public:
    /**
     * @brief Constructs an FxReader.
     *
     * @param parent Optional QObject parent.
     */
    explicit FxReader(QObject* parent = nullptr);

    /**
     * @brief Destroys the FxReader.
     */
    ~FxReader() override;

    /** @name Initialization */
    ///@{

    /**
     * @brief Opens an effect file.
     *
     * @param file Target file.
     * @param options Reader configuration.
     *
     * @return True if successful.
     */
    bool open(const core::File& file, const Options& options = Options()) override;

    /**
     * @brief Closes the effect file.
     */
    bool close() override;

    /**
     * @brief Returns true if the reader is open.
     */
    bool isOpen() const override;

    ///@}

    /** @name Data Retrieval */
    ///@{

    /**
     * @brief Returns the loaded ImageEffect.
     */
    render::ImageEffect imageEffect() const override;

    /**
     * @brief Returns current error state.
     */
    core::Error error() const override;

    ///@}

    /**
     * @brief Returns plugin handler for registration.
     */
    static plugins::PluginHandler handler();

Q_SIGNALS:

    /**
     * @brief Emitted when the reader becomes ready.
     */
    void opened();

private:
    QScopedPointer<FxReaderPrivate> p;
};

}  // namespace flipman::sdk::plugins

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::plugins::FxReader)
