// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <av/audiofilter.h>

namespace flipman::sdk::av {
class AudioFilterPrivate : public QSharedData {
public:
    struct Data {
        core::Parameters parameters;
        QString code;
        core::Error error;
    };
    Data d;
};

AudioFilter::AudioFilter()
    : p(new AudioFilterPrivate())
{}

AudioFilter::AudioFilter(const AudioFilter& other)
    : p(other.p)
{}

AudioFilter::~AudioFilter() {}

core::Parameters
AudioFilter::parameters() const
{
    return p->d.parameters;
}

QString
AudioFilter::code() const
{
    return p->d.code;
}

core::Error
AudioFilter::error() const
{
    return p->d.error;
}

void
AudioFilter::reset()
{
    p.reset(new AudioFilterPrivate());
}

void
AudioFilter::set_parameters(const core::Parameters& parameters)
{
    p->d.parameters = parameters;
}

void
AudioFilter::set_code(const QString& code)
{
    p->d.code = code;
}

AudioFilter&
AudioFilter::operator=(const AudioFilter& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

bool
AudioFilter::operator==(const AudioFilter& other) const
{
    return this->p == other.p;
}

bool
AudioFilter::operator!=(const AudioFilter& other) const
{
    return !(this->p == other.p);
}
}  // namespace flipman::sdk::av
