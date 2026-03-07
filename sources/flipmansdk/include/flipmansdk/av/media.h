// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/fps.h>
#include <flipmansdk/av/smptetime.h>
#include <flipmansdk/av/time.h>
#include <flipmansdk/av/timerange.h>
#include <flipmansdk/core/audiobuffer.h>
#include <flipmansdk/core/error.h>
#include <flipmansdk/core/file.h>
#include <flipmansdk/core/imagebuffer.h>
#include <flipmansdk/core/metadata.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QObject>

namespace flipman::sdk::av {

class MediaPrivate;

/**
 * @class Media
 * @brief Shared handle to a media resource.
 */
class FLIPMANSDK_EXPORT Media : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs an invalid Media.
     */
    Media();

    /**
     * @brief Copy constructor.
     */
    Media(const Media& other);

    /**
     * @brief Destroys the Media.
     */
    ~Media();

    /** @name Lifecycle */
    ///@{

    /**
     * @brief Opens a media file.
     */
    bool open(const core::File& file);

    /**
     * @brief Closes the media.
     */
    bool close();

    /**
     * @brief Returns true if open.
     */
    bool isOpen() const;

    /**
     * @brief Returns true if extension is supported.
     */
    bool isSupported(const QString& extension) const;

    ///@}

    /** @name Navigation */
    ///@{

    /**
     * @brief Reads the next frame.
     */
    Time read();

    /**
     * @brief Advances without decoding.
     */
    Time skip();

    /**
     * @brief Seeks to a time range.
     */
    Time seek(const TimeRange& range) const;

    ///@}

    /** @name Properties */
    ///@{

    /**
     * @brief Returns the start time.
     */
    Time start() const;

    /**
     * @brief Returns the current time.
     */
    Time time() const;

    /**
     * @brief Returns the frame rate.
     */
    Fps fps() const;

    /**
     * @brief Returns the time range.
     */
    TimeRange timeRange() const;

    /**
     * @brief Returns the associated file.
     */
    core::File file() const;

    ///@}

    /** @name Data Access */
    ///@{

    /**
     * @brief Returns the current audio buffer.
     */
    core::AudioBuffer audio() const;

    /**
     * @brief Returns the current image buffer.
     */
    core::ImageBuffer image() const;

    /**
     * @brief Returns the metadata.
     */
    core::MetaData metaData() const;

    ///@}

    /**
     * @brief Returns the error state.
     */
    core::Error error() const;

    /**
     * @brief Returns true if valid.
     */
    bool isValid() const;

    /**
     * @brief Resets to invalid state.
     */
    void reset();

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    Media& operator=(const Media& other);

    /**
     * @brief Equality operator.
     */
    bool operator==(const Media& other) const;

    /**
     * @brief Inequality operator.
     */
    bool operator!=(const Media& other) const;

    ///@}

    /** @name Wait Helpers */
    ///@{

    /**
     * @brief Waits until opened or timeout.
     */
    bool waitForOpened(int msecs = -1);

    ///@}

Q_SIGNALS:
    /**
     * @brief Emitted when media is opened.
     */
    void opened();

private:
    QExplicitlySharedDataPointer<MediaPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Media)
