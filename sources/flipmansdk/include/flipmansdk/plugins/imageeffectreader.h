// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/core/error.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/plugin.h>
#include <flipmansdk/render/imageeffect.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::plugins {

class ImageEffectReaderPrivate;

/**
 * @class ImageEffectReader
 * @brief Abstract base class for image effect file reader plugins.
 *
 * Defines the interface for loading ImageEffect objects
 * from an external file or container.
 */
class FLIPMANSDK_EXPORT ImageEffectReader : public core::Plugin {
    Q_OBJECT
public:
    /**
     * @struct Options
     * @brief Reader configuration parameters.
     *
     * Contains backend-defined attributes used when opening
     * an effect file.
     */
    struct Options {
        QVariantMap values;
    };

public:
    /**
     * @brief Constructs an ImageEffectReader.
     *
     * @param parent Optional QObject parent.
     */
    explicit ImageEffectReader(QObject* parent = nullptr);

    /**
     * @brief Destroys the ImageEffectReader.
     */
    virtual ~ImageEffectReader();

    /** @name Initialization */
    ///@{

    /**
     * @brief Opens an effect file.
     *
     * @param file Target file.
     * @param options Reader configuration.
     *
     * @return True if initialization started successfully.
     */
    virtual bool open(const core::File& file, const Options& options = Options()) = 0;

    /**
     * @brief Closes the effect file.
     *
     * @return True if successful.
     */
    virtual bool close() = 0;

    /**
     * @brief Returns true if the reader is open.
     */
    virtual bool isOpen() const = 0;

    ///@}

    /** @name Data Retrieval */
    ///@{

    /**
     * @brief Returns the loaded ImageEffect.
     */
    virtual render::ImageEffect imageEffect() const;

    /**
     * @brief Returns current error state.
     */
    virtual core::Error error() const;

    ///@}

Q_SIGNALS:

    /**
     * @brief Emitted when the reader becomes ready.
     */
    void opened();
};

}  // namespace flipman::sdk::plugins

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::plugins::ImageEffectReader)
