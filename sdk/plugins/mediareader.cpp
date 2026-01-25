// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QPointer>
#include <plugins/mediareader.h>

namespace plugins {
MediaReader::MediaReader(QObject* parent)
    : core::Plugin(parent)
{}

MediaReader::~MediaReader() {}

core::AudioBuffer
MediaReader::audio() const
{
    return core::AudioBuffer();
}

core::ImageBuffer
MediaReader::image() const
{
    return core::ImageBuffer();
}

core::Parameters
MediaReader::parameters() const
{
    return core::Parameters();
}

core::Parameters
MediaReader::metadata() const
{
    return core::Parameters();
}
}  // namespace plugins
