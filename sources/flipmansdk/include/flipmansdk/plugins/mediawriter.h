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

class MediaWriterPrivate;

/**
 * @class MediaWriter
 * @brief Abstract base class for media writing plugins.
 *
 * Defines the interface for encoding and writing image and audio
 * data to a file or container.
 */
class FLIPMANSDK_EXPORT MediaWriter : public core::Plugin {
public:
    /**
     * @struct Options
     * @brief Writer configuration parameters.
     *
     * Contains backend-defined attributes used to configure
     * encoding behavior.
     */
    struct Options {
        QVariantMap values;
    };

public:
    /**
     * @brief Constructs a MediaWriter.
     *
     * @param parent Optional QObject parent.
     */
    explicit MediaWriter(QObject* parent = nullptr);

    /**
     * @brief Destroys the MediaWriter.
     */
    virtual ~MediaWriter();

    /** @name Initialization */
    ///@{

    /**
     * @brief Opens a file for writing.
     *
     * @param file Target file.
     * @param options Encoding configuration.
     *
     * @return True if initialization succeeded.
     */
    virtual bool open(const core::File& file, const Options& options = Options()) = 0;

    /**
     * @brief Finalizes writing and closes the file.
     *
     * @return True if successful.
     */
    virtual bool close() = 0;

    /**
     * @brief Returns true if the writer is open.
     */
    virtual bool isOpen() const = 0;

    ///@}

    /** @name Capabilities */
    ///@{

    /**
     * @brief Returns true if image writing is supported.
     */
    virtual bool supportsImage() const = 0;

    /**
     * @brief Returns true if audio writing is supported.
     */
    virtual bool supportsAudio() const = 0;

    /**
     * @brief Returns supported file extensions.
     */
    virtual QList<QString> extensions() const = 0;

    ///@}

    /** @name Data Output */
    ///@{

    /**
     * @brief Writes an audio buffer.
     *
     * @return Updated presentation time.
     */
    virtual av::Time write(const core::AudioBuffer& audio);

    /**
     * @brief Writes an image buffer.
     *
     * @return Presentation time of written frame.
     */
    virtual av::Time write(const core::ImageBuffer& image);

    /**
     * @brief Seeks to a time range.
     *
     * @return Achieved time position.
     */
    virtual av::Time seek(const av::TimeRange& range);

    ///@}

    /** @name Properties */
    ///@{

    /**
     * @brief Returns current presentation time.
     */
    virtual av::Time time() const;

    /**
     * @brief Returns configured frame rate.
     */
    virtual av::Fps fps() const;

    /**
     * @brief Returns configured time range.
     */
    virtual av::TimeRange timeRange() const;

    /**
     * @brief Sets frame rate.
     */
    virtual void setFps(const av::Fps& fps);

    /**
     * @brief Sets intended time range.
     */
    virtual void setTimeRange(const av::TimeRange& timeRange);

    /**
     * @brief Sets container metadata.
     *
     * @return True if metadata was applied.
     */
    virtual bool setMetaData(const core::MetaData& metaData);

    /**
     * @brief Returns current error state.
     */
    virtual core::Error error() const;

    ///@}
};

}  // namespace flipman::sdk::plugins

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::plugins::MediaWriter)
