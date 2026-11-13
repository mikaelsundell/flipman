// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/plugins/mediawriter.h>
#include <flipmansdk/plugins/pluginhandler.h>
#include <QScopedPointer>

namespace flipman::sdk::plugins {

class QuicktimeWriterPrivate;

/**
 * @class QuicktimeWriter
 * @brief
 * MediaWriter implementation for QuickTime-compatible containers.
 *
 * Writes sequential video frames to MOV/MP4 using AVFoundation. Audio and
 * encoded timecode tracks are not supported. See docs/quicktime-writing.md.
 * Construct, write, close and destroy on one owning thread. Methods serialize
 * access; writing/finalization may block on the encoder. Use a separate writer
 * constructed on an export worker to keep encoding off the application thread.
 */
class FLIPMANSDK_EXPORT QuicktimeWriter : public plugins::MediaWriter {
public:
    /**
     * @brief
     * Constructs a QuicktimeWriter.
     *
     * @param parent Optional QObject parent.
     */
    explicit QuicktimeWriter(QObject* parent = nullptr);

    /**
     * @brief
     * Destroys the QuicktimeWriter.
     */
    ~QuicktimeWriter() override;

    /**
     * @brief
     * Opens a new MOV/MP4 file without overwriting existing files.
     *
     * Options: codec (string), bitrate (positive integer bits/sec, H.264/HEVC),
     * timeoutMs (positive integer, at most 120000; default 30000).
     *
     * @param file Target file.
     * @param options Writer configuration.
     *
     * @return True if successful.
     */
    bool open(const core::File& file, const Options& options = Options()) override;

    /**
     * @brief
     * Finalizes writing and closes the file.
     */
    bool close() override;

    /**
     * @brief
     * Returns true if the writer is open.
     */
    bool isOpen() const override;

    /**
     * @brief
     * Returns true if image encoding is supported.
     */
    bool supportsImage() const override;

    /**
     * @brief
     * Returns true if audio encoding is supported.
     */
    bool supportsAudio() const override;

    /**
     * @brief
     * Returns supported file extensions.
     */
    QList<QString> extensions() const override;

    /**
     * @brief
     * Writes an audio buffer.
     *
     * @return Updated presentation time.
     */
    av::Time write(const core::AudioBuffer& audio) override;

    /**
     * @brief
     * Appends a frame without CPU pixel conversion.
     *
     * The shared pixel storage must remain immutable until close() returns.
     * Supported input is NV12 UInt8, BGRA UInt8 or RGBA UInt8/Half/Float.
     * RGBA8 is uploaded once and packed to BGRA on Metal when native wrapping
     * is unavailable; native supported layouts retain their shared storage.
     *
     * @return Updated presentation time.
     */
    av::Time write(const core::ImageBuffer& image) override;

    /**
     * @brief
     * Seeks to a time range.
     *
     * @return Achieved time position.
     */
    av::Time seek(const av::TimeRange& range) override;

    /**
     * @brief
     * Returns current presentation time.
     */
    av::Time time() const override;

    /**
     * @brief
     * Returns configured frame rate.
     */
    av::Fps fps() const override;

    /**
     * @brief
     * Returns configured time range.
     */
    av::TimeRange timeRange() const override;

    /**
     * @brief
     * Sets frame rate.
     */
    void setFps(const av::Fps& fps) override;

    /**
     * @brief
     * Sets intended time range.
     */
    void setTimeRange(const av::TimeRange& timeRange) override;

    /**
     * @brief
     * Sets container metadata.
     *
     * @return True if metadata was applied.
     */
    bool setMetaData(const core::MetaData& metadata) override;

    /**
     * @brief
     * Returns current error state.
     */
    core::Error error() const override;

    /**
     * @brief
     * Returns plugin creator function.
     */
    static core::Plugin* creator();

    /**
     * @brief
     * Returns the registry handler for MOV and MP4 video writing.
     */
    static plugins::PluginHandler handler();

private:
    QScopedPointer<QuicktimeWriterPrivate> p;
};

}  // namespace flipman::sdk::plugins
