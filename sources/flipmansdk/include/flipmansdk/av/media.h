// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/core/audiobuffer.h>
#include <flipmansdk/core/error.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/parameters.h>
#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>

namespace flipman::sdk::av {

class MediaPrivate;

/**
 * @class Media
 * @brief Represents a shared handle to a media resource (video, audio, or image sequence).
 *
 * The Media class provides a high-level interface for accessing media data on disk.
 * It manages the lifecycle of file I/O and decoding. By using implicit sharing,
 * multiple objects can reference the same media resource efficiently.
 *
 * @note This class facilitates frame-accurate seeking and sequential reading of
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

    /**
     * @brief Destroys the media handle.
     * @note Required for the PIMPL pattern to safely delete MediaPrivate.
     */
    ~Media();

    /** @name Lifecycle Management */
    ///@{
    /**
     * @brief Opens a media file for reading.
     * @param file The file system reference to the media.
     * @return true if the file was recognized and opened successfully.
     */
    bool open(const core::File& file);

    /**
     * @brief Closes the media file and releases decoder resources.
     * @return true if the resource was closed successfully.
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
    ///@}

    /** @name Navigation and Playback */
    ///@{
    /**
     * @brief Reads the next available frame/sample from the stream.
     * @return The timestamp of the read data.
     */
    Time read();

    /**
     * @brief Increments the internal pointer to the next frame without decoding data.
     * @return The timestamp of the new position.
     */
    Time skip();

    /**
     * @brief Seeks to a specific range or timestamp within the media.
     * @param range The target time range or position.
     * @return The actual timestamp reached after seeking.
     */
    Time seek(const TimeRange& range) const;
    ///@}



    /** @name Properties */
    ///@{
    /**
     * @brief Returns the start timestamp of the media.
     */
    Time start() const;

    /**
     * @brief Returns the current playback/read position.
     */
    Time time() const;

    /**
     * @brief Returns the frame rate of the media.
     */
    Fps fps() const;

    /**
     * @brief Returns the total time range of the media.
     */
    TimeRange timeRange() const;

    /**
     * @brief Returns the file reference associated with this media.
     */
    core::File file() const;
    ///@}

    /** @name Data Access */
    ///@{
    /**
     * @brief Returns the current decoded audio buffer.
     */
    core::AudioBuffer audio() const;

    /**
     * @brief Returns the current decoded image buffer.
     */
    core::ImageBuffer image() const;

    /**
     * @brief Returns technical parameters of the stream (e.g., codec, bitrate).
     */
    core::Parameters parameters() const;

    /**
     * @brief Returns descriptive metadata (e.g., EXIF, author, creation date).
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
     * @brief Resets the media handle to a null state.
     */
    void reset();

    /** @name Operators */
    ///@{
    Media& operator=(const Media& other);
    bool operator==(const Media& other) const;
    bool operator!=(const Media& other) const;
    ///@}

private:
    QExplicitlySharedDataPointer<MediaPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Media)
