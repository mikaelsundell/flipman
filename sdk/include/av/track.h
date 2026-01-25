// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/clip.h>
#include <av/timerange.h>
#include <core/container.h>

#include <QScopedPointer>

namespace av {
class TrackPrivate;
class Track : public core::Container {
    Q_OBJECT
public:
    Track(QObject* parent = nullptr);
    virtual ~Track();
    QString name() const;
    QColor color() const;
    TimeRange cliprange(Clip* clip) const;
    QList<Clip*> clips() const;
    bool contains_clip(Clip* clip) const;
    core::Error error() const override;
    void reset() override;

    void set_name(const QString& name);
    void set_color(const QColor& color);
    void insert_clip(Clip*, const TimeRange& range);
    void remove_clip(Clip* clip);

Q_SIGNALS:
    void name_changed(const QString& name);
    void color_changed(const QColor& color);

private:
    QScopedPointer<TrackPrivate> p;
};
}  // namespace av
