// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include <QRhiWidget>
#include <QScopedPointer>
#include <av/renderlayer.h>
#include <core/imagebuffer.h>
#include <rhi/qrhi.h>

namespace widgets {
class ViewPortPrivate;
class ViewPort : public QRhiWidget {
    Q_OBJECT
public:
    ViewPort(QWidget* parent = nullptr);
    virtual ~ViewPort();
    QSize resolution() const;

    void set_background(const QColor& background);
    void set_renderlayers(const QList<av::RenderLayer> renderlayers);
    void set_resolution(const QSize& resolution);

protected:
    void initialize(QRhiCommandBuffer* cb) override;
    void render(QRhiCommandBuffer* cb) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    friend class ViewPortPrivate;
    QScopedPointer<ViewPortPrivate> p;
};
}  // namespace widgets
