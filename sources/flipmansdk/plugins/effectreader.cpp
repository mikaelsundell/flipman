// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/plugins/effectreader.h>

#include <QPointer>

namespace flipman::sdk::plugins {
EffectReader::EffectReader(QObject* parent)
    : core::Plugin(parent)
{}

EffectReader::~EffectReader() {}

render::ImageEffect
EffectReader::imageEffect() const
{
    return render::ImageEffect();
}

core::Error
EffectReader::error() const
{
    return core::Error();
}
}  // namespace flipman::sdk::plugins
