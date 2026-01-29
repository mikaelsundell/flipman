// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell.

#pragma once

#include "gamut.h"
#include <QRhiWidget>
#include <QScopedPointer>
#include <core/imagebuffer.h>
#include <rhi/qrhi.h>

class GamutWidgetPrivate;
class GamutWidget : public QRhiWidget {
    Q_OBJECT
public:
    GamutWidget(QWidget* parent = nullptr);
    virtual ~GamutWidget();
    QSize resolution() const;

    void set_background(const QColor& background);
    void set_gamuts(const QList<Gamut> gamut);
    void set_image(const core::ImageBuffer& image);
    void set_resolution(const QSize& resolution);

protected:
    void initialize(QRhiCommandBuffer* cb) override;
    void render(QRhiCommandBuffer* cb) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    friend class GamutWidgetPrivate;
    QScopedPointer<GamutWidgetPrivate> p;
};
