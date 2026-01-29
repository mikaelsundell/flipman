// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <core/audiobuffer.h>

namespace flipman::sdk::core {
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

AudioFormat
AudioBuffer::audioFormat() const
{
    return AudioFormat();
}

void
AudioBuffer::detach()
{}

bool
AudioBuffer::isValid() const
{
    return p->d.format.isValid();
}

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
}  // namespace flipman::sdk::core
