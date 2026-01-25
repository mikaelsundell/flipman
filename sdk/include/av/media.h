// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/fps.h>
#include <av/smptetime.h>
#include <av/time.h>
#include <av/timerange.h>
#include <core/audiobuffer.h>
#include <core/container.h>
#include <core/error.h>
#include <core/file.h>
#include <core/imagebuffer.h>
#include <core/parameters.h>

#include <QExplicitlySharedDataPointer>

namespace av {
class MediaPrivate;
class Media : public core::Container {
public:
    Media();
    Media(const Media& other);
    virtual ~Media();
    bool open(const core::File& file);
    bool close();
    bool is_open() const;
    bool is_supported(const QString& extension) const;
    bool is_valid() const;
    Time read();
    Time skip();
    Time seek(const TimeRange& range) const;
    Time start() const;
    Time time() const;
    Fps fps() const;
    TimeRange timerange() const;
    core::File file() const;
    core::AudioBuffer audio() const;
    core::ImageBuffer image() const;
    core::Parameters parameters() const;
    core::Parameters metadata() const;
    core::Error error() const override;
    void reset() override;

    Media& operator=(const Media& other);
    bool operator==(const Media& other) const;
    bool operator!=(const Media& other) const;

private:
    QExplicitlySharedDataPointer<MediaPrivate> p;
};
}  // namespace av
