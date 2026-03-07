// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/core/audiobuffer.h>
#include <flipmansdk/core/error.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/metadata.h>
#include <flipmansdk/core/plugin.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::plugins {

class MediaReaderPrivate;

/**
 * @class MediaReader
 * @brief Abstract base class for media reading plugins.
 *
 * Defines the interface for decoding and streaming image and audio
 * data from a file or container.
 */
class FLIPMANSDK_EXPORT MediaReader : public core::Plugin {
    Q_OBJECT
public:
    /**
     * @struct Options
     * @brief Reader configuration parameters.
     *
     * Contains backend-defined attributes used when opening media.
     */
    struct Options {
        QVariantMap values;
    };

public:
    /**
     * @brief Constructs a MediaReader.
     *
     * @param parent Optional QObject parent.
     */
    explicit MediaReader(QObject* parent = nullptr);

    /**
     * @brief Destroys the MediaReader.
     */
    virtual ~MediaReader();

    /** @name Initialization */
    ///@{

    /**
     * @brief Opens a media file.
     *
     * @param file Target file.
     * @param options Reader configuration.
     *
     * @return True if initialization started successfully.
     */
    virtual bool open(const core::File& file, const Options& options = Options()) = 0;

    /**
     * @brief Closes the media file.
     *
     * @return True if successful.
     */
    virtual bool close() = 0;

    /**
     * @brief Returns true if the reader is open.
     */
    virtual bool isOpen() const = 0;

    ///@}

    /** @name Capabilities */
    ///@{

    /**
     * @brief Returns true if image decoding is supported.
     */
    virtual bool supportsImage() const = 0;

    /**
     * @brief Returns true if audio decoding is supported.
     */
    virtual bool supportsAudio() const = 0;

    /**
     * @brief Returns supported file extensions.
     */
    virtual QList<QString> extensions() const = 0;

    ///@}

    /** @name Navigation */
    ///@{

    /**
     * @brief Reads the next frame or sample.
     *
     * @return Presentation time of decoded data.
     */
    virtual av::Time read() = 0;

    /**
     * @brief Advances without full decode.
     *
     * @return New presentation time.
     */
    virtual av::Time skip() = 0;

    /**
     * @brief Seeks to a time range.
     *
     * @return Achieved time position.
     */
    virtual av::Time seek(const av::TimeRange& range) = 0;

    ///@}

    /** @name Temporal Properties */
    ///@{

    /**
     * @brief Returns start time of the media.
     */
    virtual av::Time start() const = 0;

    /**
     * @brief Returns current time position.
     */
    virtual av::Time time() const = 0;

    /**
     * @brief Returns native frame rate.
     */
    virtual av::Fps fps() const = 0;

    /**
     * @brief Returns total time range.
     */
    virtual av::TimeRange timeRange() const = 0;

    ///@}

    /** @name Data Retrieval */
    ///@{

    /**
     * @brief Returns last decoded audio buffer.
     */
    virtual core::AudioBuffer audio() const;

    /**
     * @brief Returns last decoded image buffer.
     */
    virtual core::ImageBuffer image() const;

    /**
     * @brief Returns container metadata.
     */
    virtual core::MetaData metaData() const;

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
Q_DECLARE_METATYPE(flipman::sdk::plugins::MediaReader)
