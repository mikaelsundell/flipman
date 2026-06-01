// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/core.h>
#include <QApplication>

namespace flipman::sdk::core {

qreal
devicePixelRatio()
{
    return qApp ? qApp->devicePixelRatio() : 1.0;
}

int
physicalPixelSize(int logicalSize, qreal dpr)
{
    const qreal resolvedDpr = (dpr > 0.0) ? dpr : devicePixelRatio();
    return qMax(1, qRound(logicalSize * resolvedDpr));
}

QImage
scaledImage(const QImage& image, int logicalSize, Qt::AspectRatioMode aspectMode, Qt::TransformationMode transformMode)
{
    if (image.isNull() || logicalSize <= 0)
        return QImage();

    const qreal dpr = devicePixelRatio();
    const int physicalSize = physicalPixelSize(logicalSize, dpr);

    QImage scaled = image.scaled(physicalSize, physicalSize, aspectMode, transformMode);
    scaled.setDevicePixelRatio(dpr);
    return scaled;
}

QPixmap
scaledPixmap(const QPixmap& pixmap, int logicalSize, Qt::AspectRatioMode aspectMode,
             Qt::TransformationMode transformMode)
{
    if (pixmap.isNull() || logicalSize <= 0)
        return QPixmap();

    const qreal dpr = devicePixelRatio();
    const int physicalSize = physicalPixelSize(logicalSize, dpr);

    QPixmap scaled = pixmap.scaled(physicalSize, physicalSize, aspectMode, transformMode);
    scaled.setDevicePixelRatio(dpr);
    return scaled;
}
}  // namespace flipman::sdk::core
