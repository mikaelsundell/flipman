// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QExplicitlySharedDataPointer>
#include <core/object.h>

namespace av {
class FpsPrivate;
class Fps : public core::Object {
public:
    Fps();
    Fps(qint32 numerator, qint32 denominator, bool drop_frame = false);
    Fps(const Fps& other);
    virtual ~Fps();
    bool is_valid() const override;
    bool drop_frame() const;
    qint64 numerator() const;
    qint32 denominator() const;
    qint16 framequanta() const;
    qint32 framescale() const;
    qreal real() const;
    qreal seconds() const;
    qreal to_fps(qint64 frame, const Fps& other) const;
    QString to_string() const;
    void reset() override;

    void set_numerator(qint32 nominator);
    void set_denominator(qint32 denominator);
    void set_dropframe(bool dropframe);

    Fps& operator=(const Fps& other);
    bool operator==(const Fps& other) const;
    bool operator!=(const Fps& other) const;
    bool operator<(const Fps& other) const;
    bool operator>(const Fps& other) const;
    bool operator<=(const Fps& other) const;
    bool operator>=(const Fps& other) const;
    operator double() const;

    static Fps guess(qreal fps);
    static Fps fps_23_976();
    static Fps fps_24();
    static Fps fps_25();
    static Fps fps_29_97();
    static Fps fps_30();
    static Fps fps_47_952();
    static Fps fps_48();
    static Fps fps_50();
    static Fps fps_59_94();
    static Fps fps_60();

    static qint64 convert(quint64 value, const Fps& from, const Fps& to);

private:
    QExplicitlySharedDataPointer<FpsPrivate> p;
};
}  // namespace av
