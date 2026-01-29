// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <av/audiofilter.h>
#include <av/media.h>
#include <av/rendereffect.h>

#include <QMatrix4x4>
#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::av {

class ClipPrivate;

/**
 * @class Clip
 * @brief Represents an editable instance of media within a sequence.
 * * The Clip class extends core::Container to manage a single media asset on a timeline.
 * It encapsulates the source media, spatial transformations (2D position, scale,
 * and 3D matrices), and processing stacks for both audio and video.
 * * Clips are the primary unit for arranging content in time and space.
 */
class FLIPMANSDK_EXPORT Clip : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a new Clip.
     * @param parent The ownership parent for the QObject tree.
     */
    explicit Clip(QObject* parent = nullptr);

    /**
     * @brief Destroys the Clip and releases associated resources.
     */
    virtual ~Clip();

    /** @name Metadata and Identification
     * Basic attributes for timeline organization and display.
     */
    ///@{
    QString name() const;
    QColor color() const;
    ///@}

    /** @name Media and Processing
     * Source data and the filters/effects applied to this specific clip.
     */
    ///@{
    Media media() const;
    AudioFilter audioFilter() const;
    RenderEffect renderEffect() const;
    ///@}

    /** @name Spatial Transformations
     * Geometric attributes defining how the clip is rendered in the viewport.
     */
    ///@{
    QPointF position() const;
    QSizeF scale() const;

    /**
     * @brief Returns the full 4x4 transformation matrix for the clip.
     * This combines position, scale, rotation, and any custom shear/perspective.
     */
    QMatrix4x4 transform() const;
    ///@}

    /**
     * @brief Returns the current error state of the clip (e.g., media offline).
     */
    core::Error error() const;

    /**
     * @brief Resets all transformations and processing filters to default values.
     */
    void reset();

public Q_SLOTS:
    /** @name Setters
     * Standard slots for modifying clip attributes.
     */
    ///@{
    void setName(const QString& name);
    void setColor(const QColor& color);
    void setMedia(const Media& media);
    void setAudiofilter(const AudioFilter& audioFilter);
    void setRenderEffect(const RenderEffect& renderEffect);
    void setPosition(qreal x, qreal y);
    void setScale(qreal width, qreal height);
    void setTransform(const QMatrix4x4& matrix);
    ///@}

private:
    Q_DISABLE_COPY_MOVE(Clip)
    QScopedPointer<ClipPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Clip)
