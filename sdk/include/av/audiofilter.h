// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/container.h>
#include <core/parameters.h>

#include <QExplicitlySharedDataPointer>

namespace av {
class AudioFilterPrivate;
class AudioFilter : public core::Container {
public:
    AudioFilter();
    AudioFilter(const AudioFilter& other);
    virtual ~AudioFilter();
    core::Parameters parameters() const;
    QString code() const;
    core::Error error() const override;
    void reset() override;

    void set_parameters(const core::Parameters& parameters);
    void set_code(const QString& code);

    AudioFilter& operator=(const AudioFilter& other);
    bool operator==(const AudioFilter& other) const;
    bool operator!=(const AudioFilter& other) const;

private:
    QExplicitlySharedDataPointer<AudioFilterPrivate> p;
};
}  // namespace av
