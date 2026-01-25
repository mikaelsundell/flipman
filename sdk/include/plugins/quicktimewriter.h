// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <plugins/mediawriter.h>

#include <QScopedPointer>

class QuicktimeWriterPrivate;
class QuicktimeWriter : public plugins::MediaWriter {
public:
    QuicktimeWriter(QObject* parent = nullptr);
    virtual ~QuicktimeWriter();
    bool open(const core::File& file, core::Parameters parameters) override;
    bool close() override;
    bool is_open() const override;
    bool supports_image() const override;
    bool supports_audio() const override;
    QList<QString> extensions() const override;
    av::Time write(const core::AudioBuffer& image) override;
    av::Time write(const core::ImageBuffer& image) override;
    av::Time seek(const av::TimeRange& range) override;
    av::Time time() const override;
    av::Fps fps() const override;
    av::TimeRange timerange() const override;
    core::Error error() const override;

    void set_fps(const av::Fps& fps) override;
    void set_timerange(const av::TimeRange& timerange) override;
    bool set_metadata(const core::Parameters& parameters) override;

    static core::Plugin* creator();

private:
    QScopedPointer<QuicktimeWriterPrivate> p;
};
