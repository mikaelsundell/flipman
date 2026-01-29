// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QScopedPointer>
#include <plugins/mediareader.h>
#include <plugins/mediawriter.h>
#include <plugins/pluginhandler.h>

namespace flipman::sdk::plugins {

/**
 * @class QuicktimeReader
 * @brief Apple-native media reader implementation for QuickTime/MOV containers.
 * * This class leverages macOS/iOS native frameworks (AVFoundation/CoreMedia) to provide
 * high-performance decoding of video and audio tracks. It implements the
 * plugins::MediaReader interface for seamless integration into the flipman playback engine.
 */
class QuicktimeReaderPrivate;
class QuicktimeReader : public MediaReader {
public:
    /**
     * @brief Constructs a new QuicktimeReader.
     * @param parent The parent QObject for ownership management.
     */
    QuicktimeReader(QObject* parent = nullptr);

    /**
     * @brief Destructor. Handles cleanup of native AVFoundation resources.
     */
    ~QuicktimeReader();

    /**
     * @brief Opens a QuickTime compatible file.
     * @param file The file system path to the media.
     * @param parameters Optional configuration for the reader (e.g., hardware acceleration flags).
     * @return True if the file was successfully opened and tracks were validated.
     */
    bool open(const core::File& file, core::Parameters parameters = core::Parameters()) override;

    /**
     * @brief Closes the current file and releases native decoders.
     */
    bool close() override;

    /**
     * @brief Checks if a file is currently loaded.
     */
    bool isOpen() const override;

    /**
     * @brief Returns true if the opened file contains a valid video track.
     */
    bool supportsImage() const override;

    /**
     * @brief Returns true if the opened file contains a valid audio track.
     */
    bool supportsAudio() const override;

    /**
     * @brief Returns a list of supported file extensions (e.g., "mov", "mp4", "m4v").
     */
    QList<QString> extensions() const override;

    /**
     * @brief Reads the next frame/sample from the media stream.
     * @return The timestamp of the successfully read frame.
     */
    av::Time read() override;

    /**
     * @brief Advances the reader by one frame without decoding the image buffer.
     * @return The new current time position.
     */
    av::Time skip() override;

    /**
     * @brief Seeks to a specific time or frame range within the media.
     * @param timerange The target time point or range to jump to.
     * @return The actual time reached after the seek operation.
     */
    av::Time seek(const av::TimeRange& timerange) override;

    /**
     * @brief Returns the start time of the media (usually 0, but can vary for timecoded media).
     */
    av::Time start() const override;

    /**
     * @brief Returns the current playback position.
     */
    av::Time time() const override;

    /**
     * @brief Returns the native frame rate of the video track.
     */
    av::Fps fps() const override;

    /**
     * @brief Returns the total duration of the media.
     */
    av::TimeRange timeRange() const override;

    /**
     * @brief Retrieves the last decoded audio buffer.
     */
    core::AudioBuffer audio() const override;

    /**
     * @brief Retrieves the last decoded image buffer, typically backed by a CVPixelBuffer.
     */
    core::ImageBuffer image() const override;

    /**
     * @brief Returns the operational parameters used to open the reader.
     */
    core::Parameters parameters() const override;

    /**
     * @brief Returns file-level metadata (e.g., encoder info, creation date, track names).
     */
    core::Parameters metaData() const override;

    /**
     * @brief Returns the last error encountered by the reader.
     */
    core::Error error() const override;

    /**
     * @brief Provides a PluginHandler used by the flipman plugin system to register this reader.
     */
    static plugins::PluginHandler handler();

private:
    QScopedPointer<QuicktimeReaderPrivate> p;
};

/**
 * @class QuicktimeWriter
 * @brief Apple-native media writer implementation for QuickTime/MOV containers.
 * * This class utilizes AVFoundation's AVAssetWriter to encode video and audio streams
 * into MOV or MP4 containers. It supports hardware-accelerated encoding via 
 * VideoToolbox when available on macOS/iOS.
 */
class QuicktimeWriterPrivate;
class QuicktimeWriter : public plugins::MediaWriter {
public:
    /**
     * @brief Constructs a new QuicktimeWriter.
     * @param parent The parent QObject for ownership management.
     */
    QuicktimeWriter(QObject* parent = nullptr);

    /**
     * @brief Destructor. Finalizes any pending writes and releases AVFoundation assets.
     */
    virtual ~QuicktimeWriter();

    /**
     * @brief Initializes the asset writer for a specific file destination.
     * @param file The output file path.
     * @param parameters Encoding settings (bitrate, codec, profile).
     * @return True if the writer was successfully initialized and is ready for samples.
     */
    bool open(const core::File& file, core::Parameters parameters) override;

    /**
     * @brief Finalizes the file on disk. 
     * @note This can be a blocking operation as it finishes the interleaving of tracks.
     * @return True if the file was written successfully.
     */
    bool close() override;

    /**
     * @brief Returns true if the writer is currently active and open for writing.
     */
    bool isOpen() const override;

    /**
     * @brief Returns true if the current configuration supports video encoding.
     */
    bool supportsImage() const override;

    /**
     * @brief Returns true if the current configuration supports audio encoding.
     */
    bool supportsAudio() const override;

    /**
     * @brief Returns the list of extensions this writer can produce (e.g., "mov", "mp4").
     */
    QList<QString> extensions() const override;

    /**
     * @brief Encodes and writes an audio buffer to the container.
     * @param audio The audio sample data.
     * @return The current presentation timestamp (PTS) after the write.
     */
    av::Time write(const core::AudioBuffer& audio) override;

    /**
     * @brief Encodes and writes an image buffer (frame) to the container.
     * @param image The image buffer, ideally optimized for the underlying RHI/GPU.
     * @return The current presentation timestamp (PTS) after the write.
     */
    av::Time write(const core::ImageBuffer& image) override;

    /**
     * @brief Sets the internal clock/cursor to a specific range (for non-sequential writing).
     * @param range The target time range.
     * @return The actual time point set.
     */
    av::Time seek(const av::TimeRange& range) override;

    /**
     * @brief Returns the current presentation time of the tracks being written.
     */
    av::Time time() const override;

    /**
     * @brief Returns the target frame rate of the output file.
     */
    av::Fps fps() const override;

    /**
     * @brief Returns the expected duration or defined time range of the output.
     */
    av::TimeRange timeRange() const override;

    /**
     * @brief Configures the desired output frame rate.
     * @param fps The frames-per-second setting.
     */
    void setFps(const av::Fps& fps) override;

    /**
     * @brief Defines the total time range for the output asset.
     * @param timerange The start and duration of the file.
     */
    void setTimeRange(const av::TimeRange& timeRange) override;

    /**
     * @brief Injects metadata (author, copyright, custom tags) into the QuickTime container.
     * @param parameters A set of key-value metadata pairs.
     * @return True if the metadata was successfully set before the writing session started.
     */
    bool setMetaData(const core::Parameters& parameters) override;

    /**
     * @brief Returns the last error reported by the AVAssetWriter session.
     */
    core::Error error() const override;

    /**
     * @brief Static factory method for the plugin system to instantiate the writer.
     */
    static core::Plugin* creator();

private:
    QScopedPointer<QuicktimeWriterPrivate> p;
};

}  // namespace flipman::sdk::plugins
