// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/plugins/imageeffectreader.h>

#include <QPointer>

namespace flipman::sdk::plugins {
ImageEffectReader::ImageEffectReader(QObject* parent)
    : core::Plugin(parent)
{}

ImageEffectReader::~ImageEffectReader() {}

render::ImageEffect
ImageEffectReader::imageEffect() const
{
    return render::ImageEffect();
}

core::Error
ImageEffectReader::error() const
{
    return core::Error();
}
}  // namespace flipman::sdk::plugins
