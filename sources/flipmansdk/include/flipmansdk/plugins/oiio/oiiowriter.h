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
#include <flipmansdk/core/metadata.h>
#include <flipmansdk/plugins/mediawriter.h>
#include <flipmansdk/plugins/pluginhandler.h>

#include <QExplicitlySharedDataPointer>

namespace flipman::sdk::plugins {

class OIIOWriterPrivate;

/**
 * @class OIIOWriter
 * @brief MediaWriter implementation using OIIO facilities.
 *
 * Provides image and optional video encoding support.
 */
class OIIOWriter : public MediaWriter {
public:
    /**
     * @brief Constructs a OIIOWriter.
     *
     * @param parent Optional QObject parent.
     */
    explicit OIIOWriter(QObject* parent = nullptr);

    /**
     * @brief Destroys the OIIOWriter.
     */
    ~OIIOWriter() override;

    /**
     * @brief Opens a file for writing.
     *
     * @param file Target file.
     * @param options Writer configuration.
     *
     * @return True if successful.
     */
    bool open(const core::File& file, const Options& options = Options()) override;

    /**
     * @brief Finalizes writing and closes the file.
     */
    bool close() override;

    /**
     * @brief Returns true if the writer is open.
     */
    bool isOpen() const override;

    /**
     * @brief Returns true if image writing is supported.
     */
    bool supportsImage() const override;

    /**
     * @brief Returns true if audio writing is supported.
     */
    bool supportsAudio() const override;

    /**
     * @brief Returns supported file extensions.
     */
    QList<QString> extensions() const override;

    /**
     * @brief Writes an image buffer.
     *
     * @return Updated presentation time.
     */
    av::Time write(const core::ImageBuffer& image) override;

    /**
     * @brief Seeks to a time range.
     *
     * @return Achieved time position.
     */
    av::Time seek(const av::TimeRange& timerange) override;

    /**
     * @brief Returns current presentation time.
     */
    av::Time time() const override;

    /**
     * @brief Returns configured frame rate.
     */
    av::Fps fps() const override;

    /**
     * @brief Returns configured time range.
     */
    av::TimeRange timeRange() const override;

    /**
     * @brief Sets frame rate.
     */
    void setFps(const av::Fps& fps) override;

    /**
     * @brief Sets intended time range.
     */
    void setTimeRange(const av::TimeRange& timeRange) override;

    /**
     * @brief Sets container metadata.
     *
     * @return True if metadata was applied.
     */
    bool setMetaData(const core::MetaData& metaData) override;

    /**
     * @brief Returns current error state.
     */
    core::Error error() const override;

    /**
     * @brief Returns plugin handler for registration.
     */
    static PluginHandler handler();

private:
    QExplicitlySharedDataPointer<OIIOWriterPrivate> p;
};

}  // namespace flipman::sdk::plugins
