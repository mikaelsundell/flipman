// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QExplicitlySharedDataPointer>
#include <QMatrix4x4>
#include <av/rendereffect.h>
#include <core/imagebuffer.h>

namespace av {
class RenderLayerPrivate;
class RenderLayer : public core::Container {
public:
    RenderLayer();
    RenderLayer(const RenderLayer& other);
    virtual ~RenderLayer();
    core::ImageBuffer image() const;
    RenderEffect rendereffect() const;
    QMatrix4x4 transform() const;
    core::Error error() const override;
    void reset() override;

    void set_image(const core::ImageBuffer& image);
    void set_rendereffect(const RenderEffect& rendereffect);
    void set_transform(const QMatrix4x4& transform);

private:
    QExplicitlySharedDataPointer<RenderLayerPrivate> p;
};
}  // namespace av
