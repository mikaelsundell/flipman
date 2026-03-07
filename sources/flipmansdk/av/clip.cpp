// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/av/clip.h>

#include <QColor>
#include <QString>

namespace flipman::sdk::av {
class ClipPrivate {
public:
    void init();
    struct Data {
        QString name;
        QColor color;
        Media media;
        AudioFilter audioFilter;
        render::ImageEffect imageEffect;
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
    : QObject(parent)
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
Clip::audioFilter() const
{
    return p->d.audioFilter;
}

render::ImageEffect
Clip::imageEffect() const
{
    return p->d.imageEffect;
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
Clip::setName(const QString& name)
{
    if (p->d.name != name) {
        p->d.name = name;
    }
}

void
Clip::setColor(const QColor& color)
{
    if (p->d.color != color) {
        p->d.color = color;
    }
}

void
Clip::setMedia(const Media& media)
{
    if (p->d.media != media) {
        p->d.media = media;
    }
}

void
Clip::setAudioFilter(const AudioFilter& audioFilter)
{
    if (p->d.audioFilter != audioFilter) {
        p->d.audioFilter = audioFilter;
    }
}

void
Clip::setImageEffect(const render::ImageEffect& imageEffect)
{
    if (p->d.imageEffect != imageEffect) {
        p->d.imageEffect = imageEffect;
    }
}

void
Clip::setPosition(qreal x, qreal y)
{
    p->d.transform.setColumn(3, QVector4D(x, y, 0, 1));
}

void
Clip::setScale(qreal width, qreal height)
{
    p->d.transform.scale(width, height, 1.0);
}

void
Clip::setTransform(const QMatrix4x4& matrix)
{
    if (p->d.transform != matrix) {
        p->d.transform = matrix;
    }
}

}  // namespace flipman::sdk::av
