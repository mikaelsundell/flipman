// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/av/audiofilter.h>
#include <flipmansdk/av/media.h>
#include <flipmansdk/av/rendereffect.h>
#include <flipmansdk/flipmansdk.h>

#include <QMatrix4x4>
#include <QObject>
#include <QScopedPointer>

namespace flipman::sdk::av {

class ClipPrivate;

/**
 * @class Clip
 * @brief Represents an editable instance of media within a sequence.
 *
 * The Clip class extends QObject to manage a single media asset on a timeline.
 * It encapsulates the source media, spatial transformations (2D position, scale,
 * and 3D matrices), and processing stacks for both audio and video.
 *
 * Clips are the primary unit for arranging content in time and space.
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
    ~Clip() override;

    /** @name Metadata and Identification */
    ///@{
    /**
     * @brief Returns the display name of the clip.
     */
    QString name() const;

    /**
     * @brief Returns the timeline color associated with this clip.
     */
    QColor color() const;
    ///@}

    /** @name Media and Processing */
    ///@{
    /**
     * @brief Returns the source media asset.
     */
    Media media() const;

    /**
     * @brief Returns the current audio processing filter.
     */
    AudioFilter audioFilter() const;

    /**
     * @brief Returns the current render/video effect.
     */
    RenderEffect renderEffect() const;
    ///@}



    /** @name Spatial Transformations */
    ///@{
    /**
     * @brief Returns the 2D coordinate position on the canvas.
     */
    QPointF position() const;

    /**
     * @brief Returns the scale factors for width and height.
     */
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
    /** @name Setters */
    ///@{
    /**
     * @brief Sets the display name.
     */
    void setName(const QString& name);

    /**
     * @brief Sets the timeline color.
     */
    void setColor(const QColor& color);

    /**
     * @brief Sets the source media asset.
     */
    void setMedia(const Media& media);

    /**
     * @brief Sets the audio processing filter.
     */
    void setAudiofilter(const AudioFilter& audioFilter);

    /**
     * @brief Sets the render/video effect.
     */
    void setRenderEffect(const RenderEffect& renderEffect);

    /**
     * @brief Sets the 2D position.
     */
    void setPosition(qreal x, qreal y);

    /**
     * @brief Sets the scale factors.
     */
    void setScale(qreal width, qreal height);

    /**
     * @brief Sets the full transformation matrix.
     */
    void setTransform(const QMatrix4x4& matrix);
    ///@}

private:
    Q_DISABLE_COPY_MOVE(Clip)
    QScopedPointer<ClipPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Clip*)
