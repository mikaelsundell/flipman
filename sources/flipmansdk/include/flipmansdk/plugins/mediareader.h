// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/core/audiobuffer.h>
#include <flipmansdk/core/error.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/parameters.h>
#include <flipmansdk/core/plugin.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::plugins {

class MediaReaderPrivate;

/**
 * @class MediaReader
 * @brief Abstract base class for all media reading plugins.
 * * MediaReader defines the standard interface for decoding and streaming media data
 * (images and audio) from disk. As a core::Plugin, it allows the flipman engine
 * to support various formats (QuickTime, BRAW, OIIO) through a unified API.
 * * The interface is designed for high-performance playback, supporting
 * non-linear seeking, frame skipping, and asynchronous buffer access.
 */
class MediaReader : public core::Plugin {
public:
    /**
     * @brief Constructs a new MediaReader.
     * @param parent The parent QObject for ownership management.
     */
    MediaReader(QObject* parent = nullptr);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaReader();

    /** @name Initialization and State */
    ///@{

    /**
     * @brief Opens a media file and initializes the decoder.
     * @param file The source file path.
     * @param parameters Configuration for the decoder (e.g., resolution limits, proxy settings).
     * @return True if the file was successfully opened and tracks were validated.
     */
    virtual bool open(const core::File& file, core::Parameters parameters = core::Parameters()) = 0;

    /**
     * @brief Closes the media file and releases decoder resources.
     * @return True if the resource was released successfully.
     */
    virtual bool close() = 0;

    /**
     * @brief Checks if the reader currently has a file open.
     */
    virtual bool isOpen() const = 0;
    ///@}

    /** @name Capabilities */
    ///@{

    /**
     * @brief Returns true if the media contains decodable video/image data.
     */
    virtual bool supportsImage() const = 0;

    /**
     * @brief Returns true if the media contains decodable audio data.
     */
    virtual bool supportsAudio() const = 0;

    /**
     * @brief Returns the list of file extensions this plugin is registered to handle.
     */
    virtual QList<QString> extensions() const = 0;
    ///@}

    /** @name Navigation and Decoding */
    ///@{

    /**
     * @brief Reads the next sequential frame/sample from the stream.
     * @return The presentation timestamp (PTS) of the decoded data.
     */
    virtual av::Time read() = 0;

    /**
     * @brief Advances the internal cursor by one frame without performing a full decode.
     * @return The new time position.
     */
    virtual av::Time skip() = 0;

    /**
     * @brief Seeks to a specific time or frame range.
     * @param range The target temporal position.
     * @return The actual time achieved (closest sync frame or exact match).
     */
    virtual av::Time seek(const av::TimeRange& range) = 0;
    ///@}

    /** @name Temporal Properties */
    ///@{

    /**
     * @brief Returns the absolute start time of the media (e.g., timecode start).
     */
    virtual av::Time start() const = 0;

    /**
     * @brief Returns the current playback/cursor position.
     */
    virtual av::Time time() const = 0;

    /**
     * @brief Returns the native frame rate of the media.
     */
    virtual av::Fps fps() const = 0;

    /**
     * @brief Returns the total duration/range of the media file.
     */
    virtual av::TimeRange timeRange() const = 0;
    ///@}

    /** @name Data Retrieval */
    ///@{

    /**
     * @brief Retrieves the last decoded audio buffer.
     */
    virtual core::AudioBuffer audio() const;

    /**
     * @brief Retrieves the last decoded image buffer.
     */
    virtual core::ImageBuffer image() const;

    /**
     * @brief Returns technical metadata from the file (e.g., codec info, camera metadata).
     */
    virtual core::Parameters metaData() const;

    /**
     * @brief Returns the operational parameters used when opening the reader.
     */
    virtual core::Parameters parameters() const;

    /**
     * @brief Returns the current error state of the media.
     */
    virtual core::Error error() const;
    ///@}
};

}  // namespace flipman::sdk::plugins
