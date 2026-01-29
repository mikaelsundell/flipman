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
#include <plugins/mediawriter.h>
#include <plugins/pluginhandler.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::plugins {

class QtWriterPrivate;

/**
 * @class QtWriter
 * @brief Qt-native media writer implementation for cross-platform image and video encoding.
 * * This class provides an abstraction for writing media files using Qt's internal
 * frameworks (such as QImageWriter or Qt Multimedia). It is designed to handle
 * standard image sequences and common video formats supported natively by the
 * Qt runtime environment.
 */
class QtWriter : public MediaWriter {
public:
    /**
     * @brief Constructs a new QtWriter.
     * @param parent The parent QObject for ownership management.
     */
    QtWriter(QObject* parent = nullptr);

    /**
     * @brief Destructor. Ensures all file handles are closed and buffers flushed.
     */
    virtual ~QtWriter();

    /**
     * @brief Opens a file for writing.
     * @param file The target destination on disk.
     * @param parameters Encoding parameters such as quality, compression, or format-specific keys.
     * @return True if the file was successfully created and initialized for writing.
     */
    bool open(const core::File& file, core::Parameters parameters = core::Parameters()) override;

    /**
     * @brief Finalizes the writing process and closes the file.
     * @return True if the file was saved correctly without errors.
     */
    bool close() override;

    /**
     * @brief Checks if the writer is currently initialized and the file is open.
     */
    bool isOpen() const override;

    /**
     * @brief Returns true if the writer is configured/capable of writing image frames.
     */
    bool supportsImage() const override;

    /**
     * @brief Returns true if the writer is configured/capable of writing audio samples.
     */
    bool supportsAudio() const override;

    /**
     * @brief Returns the list of file extensions supported by this Qt-based implementation.
     */
    QList<QString> extensions() const override;

    /**
     * @brief Writes a single image frame to the media stream.
     * @param image The buffer containing pixel data to be encoded.
     * @return The presentation timestamp (PTS) of the written frame.
     */
    av::Time write(const core::ImageBuffer& image) override;

    /**
     * @brief Seeks to a specific position in the output stream (if supported by the format).
     * @param timerange The target temporal position.
     * @return The actual time reached after the seek.
     */
    av::Time seek(const av::TimeRange& timerange) override;

    /**
     * @brief Returns the current timestamp of the writer.
     */
    av::Time time() const override;

    /**
     * @brief Returns the output frame rate.
     */
    av::Fps fps() const override;

    /**
     * @brief Returns the total time range/duration of the media being written.
     */
    av::TimeRange timeRange() const override;

    /**
     * @brief Returns the last error encountered during the writing process.
     */
    core::Error error() const override;

    /**
     * @brief Sets the target frame rate for the output.
     */
    void setFps(const av::Fps& fps) override;

    /**
     * @brief Sets the intended total duration/range for the media file.
     */
    void setTimeRange(const av::TimeRange& timeRange) override;

    /**
     * @brief Applies metadata parameters (tags, comments, etc.) to the output file.
     * @return True if the metadata was successfully applied.
     */
    bool setMetaData(const core::Parameters& metaData) override;

    /**
     * @brief Provides a PluginHandler for registering this writer within the flipman SDK.
     */
    static PluginHandler handler();

private:
    /**
     * @brief Implicitly shared private data pointer for memory efficiency and d-pointer pattern.
     */
    QExplicitlySharedDataPointer<QtWriterPrivate> p;
};

}  // namespace flipman::sdk::plugins
