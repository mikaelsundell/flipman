// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.

#include <flipmansdk/av/track.h>

#include <QColor>

namespace flipman::sdk::av {
class TrackPrivate {
public:
    TrackPrivate();
    ~TrackPrivate();
    struct Data {
        QString name = "Track";
        QColor color;
        TimeRange timerange;
        QHash<Clip*, TimeRange> clips;
    };
    Data d;
};

TrackPrivate::TrackPrivate() {}

TrackPrivate::~TrackPrivate() {}

#include "track.moc"

Track::Track(QObject* parent)
    : QObject(parent)
    , p(new TrackPrivate())
{}

Track::~Track() {}

QString
Track::name() const
{
    return p->d.name;
}

QColor
Track::color() const
{
    return p->d.color;
}

TimeRange
Track::clipRange(Clip* clip) const
{
    Q_ASSERT("clip not found on track" && containsClip(clip));
    return p->d.clips.value(clip);
}

QList<Clip*>
Track::clips() const
{
    return p->d.clips.keys();
}

bool
Track::containsClip(Clip* clip) const
{
    return p->d.clips.contains(clip);
}

core::Error
Track::error() const
{
    for (Clip* clip : p->d.clips.keys()) {
        if (clip->error().hasError()) {
            return clip->error();
        }
    }
    return core::Error();
}

void
Track::reset()
{
    p.reset(new TrackPrivate());
}

void
Track::setName(const QString& name)
{
    if (p->d.name != name) {
        p->d.name = name;
        Q_EMIT nameChanged(name);
    }
}

void
Track::setColor(const QColor& color)
{
    if (p->d.color != color) {
        p->d.color = color;
        Q_EMIT colorChanged(color);
    }
}

void
Track::insertClip(Clip* clip, const TimeRange& range)
{
    /*for (const auto& existingClip : p->d.clips.keys()) {
     AVTimeRange existingRange = p->d.clips.value(existingClip);
     if (range.intersects(existingRange)) {
     qint64 ticks = existingRange.end().ticks() + 1;
     AVTimeRange adjusted(AVTime(range.start, ticks), range.duration());
     p->d.clips[clip] = adjusted;
     return;
     }
     }*/
    p->d.clips[clip] = range;
}

void
Track::removeClip(Clip* clip)
{
    Q_ASSERT("clip not found on track" && containsClip(clip));
    p->d.clips.remove(clip);
}
}  // namespace flipman::sdk::av
