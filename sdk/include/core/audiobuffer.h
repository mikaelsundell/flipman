// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/audioformat.h>
#include <core/object.h>

#include <QExplicitlySharedDataPointer>

namespace core {
class AudioBufferPrivate;
class AudioBuffer : public Object {
public:
    AudioBuffer();
    AudioBuffer(const AudioBuffer& buffer);
    virtual ~AudioBuffer();
    bool is_valid() const override;
    AudioFormat audio_format() const;
    void detach();
    void reset() override;

    AudioBuffer& operator=(const AudioBuffer& other);
    bool operator==(const AudioBuffer& other) const;
    bool operator!=(const AudioBuffer& other) const;

private:
    QExplicitlySharedDataPointer<AudioBufferPrivate> p;
};
}  // namespace core
