// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/media.h>
#include <av/time.h>
#include <av/timerange.h>
#include <core/container.h>
#include <core/error.h>

#include <QObject>
#include <QScopedPointer>

namespace av {
class MediaProcessorPrivate;
class MediaProcessor : public core::Object {
    Q_OBJECT
public:
    MediaProcessor(QObject* parent = nullptr);
    virtual ~MediaProcessor();
    bool write(Media& media, const TimeRange& timerange, const core::File& file);
    core::Error error() const;
    bool is_valid() const override;
    void reset() override;

Q_SIGNALS:
    void progress_changed(const Time& time, const TimeRange& range);

private:
    QScopedPointer<MediaProcessorPrivate> p;
};
}  // namespace av
