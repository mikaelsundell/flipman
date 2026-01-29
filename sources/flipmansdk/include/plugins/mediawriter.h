// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/fps.h>
#include <av/time.h>
#include <av/timerange.h>
#include <core/audiobuffer.h>
#include <core/error.h>
#include <core/file.h>
#include <core/imagebuffer.h>
#include <core/parameters.h>
#include <core/plugin.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::plugins {

class MediaWriterPrivate;

/**
 * @class MediaWriter
 * @brief Abstract base class for all media writing plugins.
 * * MediaWriter defines the standard interface for encoding and serializing media data
 * (images and audio) to disk. It inherits from core::Plugin to allow for dynamic
 * discovery and loading within the flipman ecosystem.
 * * Subclasses should implement format-specific logic (e.g., QuickTime, OpenImageIO, FFmpeg)
 * while adhering to the temporal and buffer-based contracts defined here.
 */
class MediaWriter : public core::Plugin {
public:
    /**
     * @brief Constructs a new MediaWriter.
     * @param parent The parent QObject for ownership management.
     */
    MediaWriter(QObject* parent = nullptr);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaWriter();

    /** @name Initialization and State */
    ///@{

    /**
     * @brief Opens a file for writing and initializes the encoder.
     * @param file The target file path.
     * @param parameters Configuration for the encoder (e.g., bitrate, compression level).
     * @return True if the writer was successfully initialized.
     */
    virtual bool open(const core::File& file, core::Parameters parameters = core::Parameters()) = 0;

    /**
     * @brief Finalizes the writing process and closes the file handle.
     * @return True if the file was written and closed successfully.
     */
    virtual bool close() = 0;

    /**
     * @brief Checks if the writer is currently active and a file is open for writing.
     * @return True if the writing session is valid.
     */
    virtual bool isOpen() const = 0;
    ///@}

    /** @name Capabilities */
    ///@{

    /**
     * @brief Indicates if this writer supports video/image sequence encoding.
     */
    virtual bool supportsImage() const = 0;

    /**
     * @brief Indicates if this writer supports audio stream encoding.
     */
    virtual bool supportsAudio() const = 0;

    /**
     * @brief Returns the list of file extensions this plugin is capable of writing.
     * @return A list of strings (e.g., {"mov", "mp4", "exr"}).
     */
    virtual QList<QString> extensions() const = 0;
    ///@}

    /** @name Data Output */
    ///@{

    /**
     * @brief Encodes and writes an audio buffer to the output stream.
     * @param audio The audio samples to write.
     * @return The updated presentation time after writing.
     */
    virtual av::Time write(const core::AudioBuffer& audio);

    /**
     * @brief Encodes and writes an image buffer (frame) to the output stream.
     * @param image The pixel data buffer to write.
     * @return The presentation time associated with the written frame.
     */
    virtual av::Time write(const core::ImageBuffer& image);

    /**
     * @brief Moves the writer's cursor to a specific time range.
     * @param range The target time range to seek to.
     * @return The actual time position achieved.
     */
    virtual av::Time seek(const av::TimeRange& range);
    ///@}

    /** @name Properties and Metadata */
    ///@{

    /**
     * @brief Returns the current presentation time of the writer.
     */
    virtual av::Time time() const;

    /**
     * @brief Returns the configured frame rate of the output.
     */
    virtual av::Fps fps() const;

    /**
     * @brief Returns the total time range (duration) of the media being written.
     */
    virtual av::TimeRange timeRange() const;

    /**
     * @brief Sets the target frame rate for the encoding session.
     * @param fps The desired frames per second.
     */
    virtual void setFps(const av::Fps& fps);

    /**
     * @brief Sets the intended total time range for the output file.
     * @param timerange The start and duration of the file.
     */
    virtual void setTimeRange(const av::TimeRange& timeRange);

    /**
     * @brief Applies metadata (tags, comments, etc.) to the output container.
     * @param metadata A set of parameters representing metadata keys and values.
     * @return True if the metadata was successfully applied.
     */
    virtual bool setMetaData(const core::Parameters& metaData);

    /**
     * @brief Returns the current error state of the media.
     */
    virtual core::Error error() const;
    ///@}
};

}  // namespace flipman::sdk::plugins
