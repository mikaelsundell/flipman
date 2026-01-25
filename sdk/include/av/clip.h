// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <av/audiofilter.h>
#include <av/media.h>
#include <av/rendereffect.h>
#include <core/container.h>

#include <QMatrix4x4>
#include <QObject>
#include <QScopedPointer>

namespace av {
class ClipPrivate;
class Clip : public core::Container {
    Q_OBJECT
public:
    Clip(QObject* parent = nullptr);
    virtual ~Clip();
    QString name() const;
    QColor color() const;
    Media media() const;
    AudioFilter audiofilter() const;
    RenderEffect rendereffect() const;
    QPointF position() const;
    QSizeF scale() const;
    QMatrix4x4 transform() const;
    core::Error error() const override;
    void reset() override;

    void set_name(const QString& name);
    void set_color(const QColor& color);
    void set_media(const Media& media);
    void set_audiofilter(const AudioFilter& audiofilter);
    void set_rendereffect(const RenderEffect& rendereffect);
    void set_position(qreal x, qreal y);
    void set_scale(qreal width, qreal height);
    void set_transform(const QMatrix4x4& matrix);

private:
    QScopedPointer<ClipPrivate> p;
};
}  // namespace av
