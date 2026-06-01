// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>
#include <QMetaType>

namespace flipman::sdk::core {

/** @name Qt / Image Conversions */
///@{
/**
     * @brief Returns the current application device pixel ratio.
     *
     * This helper returns the device pixel ratio from the active QApplication
     * when available, and falls back to 1.0 when no application instance exists.
     *
     * @return The current device pixel ratio, or 1.0 if unavailable.
     */
qreal
devicePixelRatio();

/**
     * @brief Converts a logical UI size into a physical pixel size.
     *
     * Multiplies the supplied logical size by the given device pixel ratio and
     * rounds the result to an integer pixel size suitable for raster scaling.
     * If @p dpr is 0.0 or less, the current application device pixel ratio is used.
     *
     * @param logicalSize The logical size in device-independent pixels.
     * @param dpr Optional device pixel ratio override.
     * @return The corresponding physical pixel size, clamped to at least 1.
     */
int
physicalPixelSize(int logicalSize, qreal dpr = 0.0);

/**
     * @brief Scales an image to a logical UI size with device-pixel-ratio support.
     *
     * The image is scaled to a square raster size derived from @p logicalSize and
     * the current application device pixel ratio, then tagged with that device
     * pixel ratio so it renders at the intended logical size in Qt views.
     *
     * @param image Source image to scale.
     * @param logicalSize Target logical size in device-independent pixels.
     * @param aspectMode Aspect ratio policy used during scaling.
     * @param transformMode Transformation quality used during scaling.
     * @return A scaled image with device pixel ratio set, or a null image on failure.
     */
QImage
scaledImage(const QImage& image, int logicalSize, Qt::AspectRatioMode aspectMode,
            Qt::TransformationMode transformMode = Qt::SmoothTransformation);

/**
     * @brief Scales a pixmap to a logical UI size with device-pixel-ratio support.
     *
     * The pixmap is scaled to a square raster size derived from @p logicalSize and
     * the current application device pixel ratio, then tagged with that device
     * pixel ratio so it renders at the intended logical size in Qt widgets.
     *
     * @param pixmap Source pixmap to scale.
     * @param logicalSize Target logical size in device-independent pixels.
     * @param aspectMode Aspect ratio policy used during scaling.
     * @param transformMode Transformation quality used during scaling.
     * @return A scaled pixmap with device pixel ratio set, or a null pixmap on failure.
     */
QPixmap
scaledPixmap(const QPixmap& pixmap, int logicalSize, Qt::AspectRatioMode aspectMode,
             Qt::TransformationMode transformMode = Qt::SmoothTransformation);


}  // namespace flipman::sdk::core
