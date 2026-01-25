// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <QExplicitlySharedDataPointer>
#include <av/fps.h>
#include <av/time.h>
#include <av/timerange.h>
#include <core/audiobuffer.h>
#include <core/error.h>
#include <core/file.h>
#include <core/imagebuffer.h>
#include <core/parameters.h>
#include <plugins/mediawriter.h>
#include <plugins/pluginhandler.h>

class QtWriterPrivate;
class QtWriter : public plugins::MediaWriter {
public:
    QtWriter(QObject* parent = nullptr);
    virtual ~QtWriter();
    bool open(const core::File& file, core::Parameters parameters = core::Parameters()) override;
    bool close() override;
    bool is_open() const override;
    bool supports_image() const override;
    bool supports_audio() const override;
    QList<QString> extensions() const override;
    av::Time write(const core::ImageBuffer& image) override;
    av::Time seek(const av::TimeRange& timerange) override;
    av::Time time() const override;
    av::Fps fps() const override;
    av::TimeRange timerange() const override;
    core::Error error() const override;

    void set_fps(const av::Fps& fps) override;
    void set_timerange(const av::TimeRange& timerange) override;
    bool set_metadata(const core::Parameters& metadata) override;

    static plugins::PluginHandler handler();

private:
    QExplicitlySharedDataPointer<QtWriterPrivate> p;
};
