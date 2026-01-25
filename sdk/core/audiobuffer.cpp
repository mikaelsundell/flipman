// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <core/audiobuffer.h>

namespace core {
class AudioBufferPrivate : public QSharedData {
public:
    AudioBufferPrivate();
    ~AudioBufferPrivate();
    struct Data {
        AudioFormat format;
    };
    Data d;
};

AudioBufferPrivate::AudioBufferPrivate() {}

AudioBufferPrivate::~AudioBufferPrivate() {}

AudioBuffer::AudioBuffer()
    : p(new AudioBufferPrivate())
{}

AudioBuffer::AudioBuffer(const AudioBuffer& other)
    : p(other.p)
{}

AudioBuffer::~AudioBuffer() {}

bool
AudioBuffer::is_valid() const
{
    return p->d.format.is_valid();
}

AudioFormat
AudioBuffer::audio_format() const
{
    return AudioFormat();
}

void
AudioBuffer::detach()
{}

void
AudioBuffer::reset()
{}

AudioBuffer&
AudioBuffer::operator=(const AudioBuffer& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
AudioBuffer::operator==(const AudioBuffer& other) const
{
    return p == other.p;
}

bool
AudioBuffer::operator!=(const AudioBuffer& other) const
{
    return !(*this == other);
}
}  // namespace core
