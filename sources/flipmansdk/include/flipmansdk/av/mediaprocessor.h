// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/media.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/core/error.h>

#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::av {

class MediaProcessorPrivate;

/**
 * @class MediaProcessor
 * @brief Processes Media resources to disk.
 */
class FLIPMANSDK_EXPORT MediaProcessor : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a MediaProcessor.
     */
    explicit MediaProcessor(QObject* parent = nullptr);

    /**
     * @brief Destroys the MediaProcessor.
     */
    ~MediaProcessor() override;

    /** @name Processing */
    ///@{

    /**
     * @brief Writes a time range to a file.
     */
    bool write(Media& media, const TimeRange& timerange, const core::File& file);

    /**
     * @brief Resets the processor state.
     */
    void reset();

    ///@}

    /** @name Status */
    ///@{

    /**
     * @brief Returns the error state.
     */
    core::Error error() const;

    /**
     * @brief Returns true if valid.
     */
    bool isValid() const;

    ///@}

Q_SIGNALS:
    /**
     * @brief Emitted when progress changes.
     */
    void progressChanged(const sdk::av::Time& time, const sdk::av::TimeRange& range);

    /**
     * @brief Emitted when processing finishes.
     */
    void finished();

private:
    Q_DISABLE_COPY_MOVE(MediaProcessor)
    QScopedPointer<MediaProcessorPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::MediaProcessor*)
