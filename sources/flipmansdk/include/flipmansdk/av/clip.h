// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <flipmansdk/av/audiofilter.h>
#include <flipmansdk/av/media.h>
#include <flipmansdk/render/imageeffect.h>

#include <QColor>
#include <QMatrix4x4>
#include <QObject>
#include <QPointF>
#include <QScopedPointer>
#include <QSizeF>

namespace flipman::sdk::av {

class ClipPrivate;

/**
 * @class Clip
 * @brief Editable media instance placed on a timeline.
 */
class FLIPMANSDK_EXPORT Clip : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructs a Clip.
     */
    explicit Clip(QObject* parent = nullptr);

    /**
     * @brief Destroys the Clip.
     */
    ~Clip() override;

    /** @name Metadata */
    ///@{

    /**
     * @brief Returns the clip display name.
     */
    QString name() const;

    /**
     * @brief Returns the clip timeline color.
     */
    QColor color() const;

    ///@}

    /** @name Media and Processing */
    ///@{

    /**
     * @brief Returns the associated media.
     */
    Media media() const;

    /**
     * @brief Returns the audio filter.
     */
    AudioFilter audioFilter() const;

    /**
     * @brief Returns the image effect.
     */
    render::ImageEffect imageEffect() const;

    ///@}

    /** @name Spatial Transform */
    ///@{

    /**
     * @brief Returns the 2D position.
     */
    QPointF position() const;

    /**
     * @brief Returns the 2D scale.
     */
    QSizeF scale() const;

    /**
     * @brief Returns the transformation matrix.
     */
    QMatrix4x4 transform() const;

    ///@}

    /**
     * @brief Returns the current error state.
     */
    core::Error error() const;

    /**
     * @brief Resets the clip to default state.
     */
    void reset();

public Q_SLOTS:
    /** @name Setters */
    ///@{

    /**
     * @brief Sets the clip display name.
     */
    void setName(const QString& name);

    /**
     * @brief Sets the clip timeline color.
     */
    void setColor(const QColor& color);

    /**
     * @brief Sets the associated media.
     */
    void setMedia(const Media& media);

    /**
     * @brief Sets the audio filter.
     */
    void setAudioFilter(const AudioFilter& audioFilter);

    /**
     * @brief Sets the image effect.
     */
    void setImageEffect(const render::ImageEffect& imageEffect);

    /**
     * @brief Sets the 2D position.
     */
    void setPosition(qreal x, qreal y);

    /**
     * @brief Sets the 2D scale.
     */
    void setScale(qreal width, qreal height);

    /**
     * @brief Sets the transformation matrix.
     */
    void setTransform(const QMatrix4x4& matrix);

    ///@}

private:
    Q_DISABLE_COPY_MOVE(Clip)
    QScopedPointer<ClipPrivate> p;
};

}  // namespace flipman::sdk::av

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::av::Clip*)
