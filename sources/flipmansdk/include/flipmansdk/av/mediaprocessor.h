// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/media.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/core/error.h>
#include <flipmansdk/flipmansdk.h>

#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::av {

class MediaProcessorPrivate;

/**
 * @class MediaProcessor
 * @brief Handles the transcoding, exporting, and rendering of Media resources to disk.
 *
 * MediaProcessor provides a high-level interface for processing media. It allows
 * for extracting specific time ranges from a Media source and writing them to
 * a new destination file. The processing happens asynchronously, reporting
 * progress via signals.
 */
class FLIPMANSDK_EXPORT MediaProcessor : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a MediaProcessor.
     * @param parent The ownership parent.
     */
    explicit MediaProcessor(QObject* parent = nullptr);

    /**
     * @brief Destroys the MediaProcessor.
     * @note Required for the PIMPL pattern to safely delete MediaProcessorPrivate.
     */
    ~MediaProcessor() override;

    /** @name Processing */
    ///@{
    /**
     * @brief Writes a specified range of media to a destination file.
     *
     * This method initiates the render/export process. Depending on the implementation,
     * this may trigger an internal thread or utilize background task scheduling.
     *
     * @param media The source media to process.
     * @param timerange The specific segment to export.
     * @param file The destination file path and format.
     * @return true if the process started successfully.
     */
    bool write(Media& media, const TimeRange& timerange, const core::File& file);

    /**
     * @brief Cancels any active processing and resets the processor state.
     */
    void reset();
    ///@}



    /** @name Status */
    ///@{
    /**
     * @brief Returns the last error encountered during processing.
     */
    core::Error error() const;

    /**
     * @brief Validates if the processor is in a state ready for execution.
     */
    bool isValid() const;
    ///@}

Q_SIGNALS:
    /**
     * @brief Emitted periodically to report the current progress of the write operation.
     * @param time The current timestamp being processed.
     * @param range The total range being processed for percentage calculation.
     */
    void progressChanged(const sdk::av::Time& time, const sdk::av::TimeRange& range);

    /**
     * @brief Emitted when the processing job has finished successfully.
     */
    void finished();

private:
    Q_DISABLE_COPY_MOVE(MediaProcessor)
    QScopedPointer<MediaProcessorPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::MediaProcessor*)
