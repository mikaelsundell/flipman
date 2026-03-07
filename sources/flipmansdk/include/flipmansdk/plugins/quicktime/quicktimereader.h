// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/pluginhandler.h>

#include <QScopedPointer>

namespace flipman::sdk::plugins {

class QuicktimeReaderPrivate;

/**
 * @class QuicktimeReader
 * @brief MediaReader implementation for QuickTime-compatible containers.
 *
 * Provides decoding of image and audio streams.
 */
class QuicktimeReader : public MediaReader {
public:
    /**
     * @brief Constructs a QuicktimeReader.
     *
     * @param parent Optional QObject parent.
     */
    explicit QuicktimeReader(QObject* parent = nullptr);

    /**
     * @brief Destroys the QuicktimeReader.
     */
    ~QuicktimeReader() override;

    /**
     * @brief Opens a media file.
     *
     * @param file Target file.
     * @param options Reader configuration.
     *
     * @return True if successful.
     */
    bool open(const core::File& file, const Options& options = Options()) override;

    /**
     * @brief Closes the media file.
     */
    bool close() override;

    /**
     * @brief Returns true if a file is open.
     */
    bool isOpen() const override;

    /**
     * @brief Returns true if image decoding is available.
     */
    bool supportsImage() const override;

    /**
     * @brief Returns true if audio decoding is available.
     */
    bool supportsAudio() const override;

    /**
     * @brief Returns supported file extensions.
     */
    QList<QString> extensions() const override;

    /**
     * @brief Reads the next frame or sample.
     *
     * @return Presentation time of decoded data.
     */
    av::Time read() override;

    /**
     * @brief Advances without full decode.
     *
     * @return New presentation time.
     */
    av::Time skip() override;

    /**
     * @brief Seeks to a time range.
     *
     * @return Achieved time position.
     */
    av::Time seek(const av::TimeRange& timerange) override;

    /**
     * @brief Returns start time of the media.
     */
    av::Time start() const override;

    /**
     * @brief Returns current playback position.
     */
    av::Time time() const override;

    /**
     * @brief Returns native frame rate.
     */
    av::Fps fps() const override;

    /**
     * @brief Returns total time range.
     */
    av::TimeRange timeRange() const override;

    /**
     * @brief Returns last decoded audio buffer.
     */
    core::AudioBuffer audio() const override;

    /**
     * @brief Returns last decoded image buffer.
     */
    core::ImageBuffer image() const override;

    /**
     * @brief Returns container metadata.
     */
    core::MetaData metaData() const override;

    /**
     * @brief Returns current error state.
     */
    core::Error error() const override;

    /**
     * @brief Returns the plugin handler for registration.
     */
    static plugins::PluginHandler handler();

private:
    QScopedPointer<QuicktimeReaderPrivate> p;
};

}  // namespace flipman::sdk::plugins
