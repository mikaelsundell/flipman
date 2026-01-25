// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QFile>
#include <QObject>
#include <QScopedPointer>
#include <av/renderlayer.h>
#include <core/object.h>
#include <rhi/qrhi.h>

namespace av {
class RenderEnginePrivate;
class RenderEngine : public core::Object {
    Q_OBJECT
public:
    RenderEngine(QObject* parent = nullptr);
    virtual ~RenderEngine();
    void init(QRhiCommandBuffer* cb);
    void render(QRhiCommandBuffer* cb);
    QSize resolution() const;

    void set_background(const QColor& background);
    void set_renderlayers(const QList<RenderLayer> renderlayers);
    void set_resolution(const QSize& resolution);

    static QShader compile(const QFile& file);

private:
    QScopedPointer<RenderEnginePrivate> p;
};
}  // namespace av
