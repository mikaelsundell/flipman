// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QScopedPointer>
#include <flipmansdk/plugins/mediareader.h>
#include <flipmansdk/plugins/pluginhandler.h>

namespace flipman::sdk::plugins {

class BrawReaderPrivate;

/**
 * @class BrawReader
 * @brief CPU Blackmagic RAW decoder adapted from brawtool.
 *
 * Decodes RGB float32 frames with clip processing defaults and post-3D-LUT
 * disabled. Audio is not exposed. Preview processing belongs to the pipeline.
 * Calls on one reader are serialized; independent readers use separate codecs.
 * read() returns the decoded frame time and advances the next-frame cursor.
 * EOF returns invalid time without error and clears the image.
 * seek() limits subsequent reads to the supplied range within the full clip.
 * Runtime lookup: app/executable-relative framework, FLIPMAN_BRAW_RUNTIME_DIR (framework parent), then
 * SDK-relative Libraries inferred from BLACKMAGIC_RAW_ROOT. The SDK dispatch
 * caches its first successfully loaded runtime for the process lifetime.
 * The reader reports an error when built without FLIPMAN_BUILD_BRAW.
 * Options values: kelvin (2000..50000), tint (-150..150), exposure (-20..20).
 * Overrides affect decoded frames in memory; no sidecar is written.
 */
class FLIPMANSDK_EXPORT BrawReader : public MediaReader {
public:
    /**
     * @brief Constructs a BrawReader.
     *
     * @param parent Optional QObject parent.
     */
    explicit BrawReader(QObject* parent = nullptr);

    /**
     * @brief Destroys the BrawReader.
     */
    ~BrawReader() override;

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
     * @brief Returns false; audio decoding is not implemented.
     */
    bool supportsAudio() const override;

    /**
     * @brief Returns true if multiple reader instances may decode frames
     * concurrently from the same media source.
     */
    bool supportsConcurrent() const override;

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
     * @brief Returns the next frame cursor (after read, one frame beyond the returned time).
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
    QScopedPointer<BrawReaderPrivate> p;
};

}  // namespace flipman::sdk::plugins
