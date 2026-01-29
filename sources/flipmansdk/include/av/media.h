// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <av/fps.h>
#include <av/smptetime.h>
#include <av/time.h>
#include <av/timerange.h>
#include <core/audiobuffer.h>
#include <core/error.h>
#include <core/file.h>
#include <core/imagebuffer.h>
#include <core/parameters.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::av {

class MediaPrivate;

/**
 * @class Media
 * @brief Represents a shared handle to a media resource (video, audio, or image sequence).
 * * The Media class provides a high-level interface for accessing media data on disk.
 * It manages the lifecycle of file I/O and decoding. By using implicit sharing,
 * multiple objects can reference the same media resource efficiently.
 * * @note This class facilitates frame-accurate seeking and sequential reading of
 * both image and audio buffers.
 */
class FLIPMANSDK_EXPORT Media {
public:
    /**
     * @brief Constructs an empty, invalid Media object.
     */
    Media();

    /**
     * @brief Copy constructor. Performs a shallow copy of the media handle.
     */
    Media(const Media& other);

    virtual ~Media();

    /** @name Lifecycle Management
     * Methods for managing the connection to the physical file.
     */
    ///@{
    /**
     * @brief Opens a media file for reading.
     * @param file The file system reference to the media.
     * @return true if the file was recognized and opened successfully.
     */
    bool open(const core::File& file);

    /**
     * @brief Closes the media file and releases decoder resources.
     */
    bool close();

    /**
     * @brief Returns true if the media is currently open and accessible.
     */
    bool isOpen() const;

    /**
     * @brief Checks if the given file extension is supported by the SDK's backends.
     */
    bool isSupported(const QString& extension) const;

    /** @name Navigation and Playback
     * Logic for traversing the media timeline.
     */
    ///@{
    /**
     * @brief Reads the next available frame/sample from the stream.
     * @return The timestamp of the read data.
     */
    Time read();

    /**
     * @brief Increments the internal pointer to the next frame without decoding data.
     */
    Time skip();

    /**
     * @brief Seeks to a specific range or timestamp within the media.
     * @return The actual timestamp reached after seeking.
     */
    Time seek(const TimeRange& range) const;
    ///@}

    /** @name Properties
     * Technical details about the opened media resource.
     */
    ///@{
    Time start() const;
    Time time() const;
    Fps fps() const;
    TimeRange timeRange() const;
    core::File file() const;
    ///@}

    /** @name Data Access
     * Methods to retrieve the actual decoded payloads.
     */
    ///@{
    core::AudioBuffer audio() const;
    core::ImageBuffer image() const;

    /**
     * @brief Technical parameters of the stream (e.g., codec, bitrate, pixel format).
     */
    core::Parameters parameters() const;

    /**
     * @brief Descriptive metadata (e.g., EXIF, XMP, author, creation date).
     */
    core::Parameters metaData() const;
    ///@}

    /**
     * @brief Returns the current error state of the media.
     */
    core::Error error() const;

    /**
     * @brief Returns true if the media has valid tracks (video or audio).
     */
    bool isValid() const;

    /**
     * @brief Resets all transformations and processing filters to default values.
     */
    void reset();

    /** @name Operators */
    ///@{
    Media& operator=(const Media& other);
    bool operator==(const Media& other) const;
    bool operator!=(const Media& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<MediaPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Media)
