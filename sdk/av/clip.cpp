// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <av/clip.h>

#include <QColor>

namespace av {
class ClipPrivate {
public:
    void init();
    struct Data {
        QString name;
        QColor color;
        Media media;
        AudioFilter audiofilter;
        RenderEffect rendereffect;
        QMatrix4x4 transform;
    };
    Data d;
};

void
ClipPrivate::init()
{
    d.transform.setToIdentity();
}

Clip::Clip(QObject* parent)
    : core::Container(parent)
    , p(new ClipPrivate())
{
    p->init();
}

Clip::~Clip() {}

QString
Clip::name() const
{
    return p->d.name;
}

QColor
Clip::color() const
{
    return p->d.color;
}

Media
Clip::media() const
{
    return p->d.media;
}

AudioFilter
Clip::audiofilter() const
{
    return p->d.audiofilter;
}

RenderEffect
Clip::rendereffect() const
{
    return p->d.rendereffect;
}

QPointF
Clip::position() const
{
    return QPointF(p->d.transform(0, 3), p->d.transform(1, 3));
}

QSizeF
Clip::scale() const
{
    return QSizeF(p->d.transform(0, 0), p->d.transform(1, 1));
}

QMatrix4x4
Clip::transform() const
{
    return p->d.transform;
}

core::Error
Clip::error() const
{
    return p->d.media.error();
}

void
Clip::reset()
{
    p.reset(new ClipPrivate());
    p->init();
}

void
Clip::set_name(const QString& name)
{
    if (p->d.name != name) {
        p->d.name = name;
    }
}

void
Clip::set_color(const QColor& color)
{
    if (p->d.color != color) {
        p->d.color = color;
    }
}

void
Clip::set_media(const Media& media)
{
    if (p->d.media != media) {
        p->d.media = media;
    }
}

void
Clip::set_audiofilter(const AudioFilter& audiofilter)
{
    if (p->d.audiofilter != audiofilter) {
        p->d.audiofilter = audiofilter;
    }
}

void
Clip::set_rendereffect(const RenderEffect& rendereffect)
{
    if (p->d.rendereffect != rendereffect) {
        p->d.rendereffect = rendereffect;
    }
}

void
Clip::set_position(qreal x, qreal y)
{
    p->d.transform.setColumn(3, QVector4D(x, y, 0, 1));
}

void
Clip::set_scale(qreal width, qreal height)
{
    p->d.transform.scale(width, height, 1.0);
}

void
Clip::set_transform(const QMatrix4x4& matrix)
{
    if (p->d.transform != matrix) {
        p->d.transform = matrix;
    }
}

}  // namespace av
