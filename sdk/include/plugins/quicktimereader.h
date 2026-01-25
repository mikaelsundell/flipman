// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QScopedPointer>
#include <plugins/mediareader.h>
#include <plugins/pluginhandler.h>

class QuicktimeReaderPrivate;
class QuicktimeReader : public plugins::MediaReader {
public:
    QuicktimeReader(QObject* parent = nullptr);
    ~QuicktimeReader();
    bool open(const core::File& file, core::Parameters parameters = core::Parameters()) override;
    bool close() override;
    bool is_open() const override;
    bool supports_image() const override;
    bool supports_audio() const override;
    QList<QString> extensions() const override;
    av::Time read() override;
    av::Time skip() override;
    av::Time seek(const av::TimeRange& timerange) override;
    av::Time start() const override;
    av::Time time() const override;
    av::Fps fps() const override;
    av::TimeRange timerange() const override;
    core::AudioBuffer audio() const override;
    core::ImageBuffer image() const override;
    core::Parameters parameters() const override;
    core::Parameters metadata() const override;
    core::Error error() const override;

    static plugins::PluginHandler handler();

private:
    QScopedPointer<QuicktimeReaderPrivate> p;
};
