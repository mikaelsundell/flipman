// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/av/audiofilter.h>

namespace flipman::sdk::av {
class AudioFilterPrivate : public QSharedData {
public:
    struct Data {
        QString data;
        core::MetaData metaData;
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

core::MetaData
AudioFilter::metaData() const
{
    return p->d.metaData;
}

QString
AudioFilter::data() const
{
    return p->d.data;
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
AudioFilter::setMetaData(const core::MetaData& metaData)
{
    p->d.metaData = metaData;
}

void
AudioFilter::setData(const QString& data)
{
    p->d.data = data;
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
