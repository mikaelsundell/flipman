
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/object.h>

#include <QExplicitlySharedDataPointer>
#include <QString>

namespace core {
class AudioFormatPrivate;
class AudioFormat : public Object {
public:
    AudioFormat();
    AudioFormat(const AudioFormat& format);
    virtual ~AudioFormat();
    bool is_valid() const override;
    void reset() override;

    AudioFormat& operator=(const AudioFormat& other);
    bool operator==(const AudioFormat& other) const;
    bool operator!=(const AudioFormat& other) const;

private:
    QExplicitlySharedDataPointer<AudioFormatPrivate> p;
};
}  // namespace core
