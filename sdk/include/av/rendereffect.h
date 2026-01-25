// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/container.h>
#include <core/parameters.h>

#include <QExplicitlySharedDataPointer>

namespace av {
class RenderEffectPrivate;
class RenderEffect : public core::Container {
public:
    RenderEffect();
    RenderEffect(const RenderEffect& other);
    virtual ~RenderEffect();
    core::Parameters parameters() const;
    QString code() const;
    core::Error error() const override;
    void reset() override;

    void set_parameters(const core::Parameters& parameters);
    void set_code(const QString& code);

    RenderEffect& operator=(const RenderEffect& other);
    bool operator==(const RenderEffect& other) const;
    bool operator!=(const RenderEffect& other) const;

private:
    QExplicitlySharedDataPointer<RenderEffectPrivate> p;
};
}  // namespace av
