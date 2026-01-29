// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <core/audioformat.h>

namespace flipman::sdk::core {
class AudioFormatPrivate : public QSharedData {
public:
    AudioFormatPrivate();
    ~AudioFormatPrivate();
    struct Data {
        int type;
    };
    Data d;
};

AudioFormatPrivate::AudioFormatPrivate() {}

AudioFormatPrivate::~AudioFormatPrivate() {}

AudioFormat::AudioFormat() {}

AudioFormat::AudioFormat(const AudioFormat& format) {}

AudioFormat::~AudioFormat() {}

bool
AudioFormat::isValid() const
{
    return p->d.type > 0;
}

void
AudioFormat::reset()
{
    p.detach();
    p->d.type = 0;
}

AudioFormat&
AudioFormat::operator=(const AudioFormat& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
AudioFormat::operator==(const AudioFormat& other) const
{
    return p->d.type == other.p->d.type;
}

bool
AudioFormat::operator!=(const AudioFormat& other) const
{
    return !(*this == other);
}

}  // namespace flipman::sdk::core
