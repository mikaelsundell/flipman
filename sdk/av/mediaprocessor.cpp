// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QtConcurrent>
#include <av/mediaprocessor.h>
#include <plugins/mediawriter.h>
#include <plugins/pluginregistry.h>

namespace av {
class MediaProcessorPrivate {
public:
    struct Data {
        core::Error error;
    };
    Data d;
};

MediaProcessor::MediaProcessor(QObject* parent)
    : core::Object(parent)
    , p(new MediaProcessorPrivate())
{}

MediaProcessor::~MediaProcessor() {}

bool
MediaProcessor::write(Media& media, const TimeRange& timerange, const core::File& file)
{
    plugins::PluginRegistry* registry = plugins::PluginRegistry::instance();
    QScopedPointer<plugins::MediaWriter> writer(registry->get_plugin<plugins::MediaWriter>(file.extension()));
    if (writer) {
        writer->open(file);
        writer->set_timerange(timerange);
        av::Time time = media.seek(timerange);
        for (qint64 frame = timerange.start().frames(); frame < timerange.duration().frames(); frame++) {
            qDebug() << "write frame: " << frame;

            av::Time next = av::Time(frame, media.fps());
            if (time < next || frame == timerange.start().frames()) {
                time = media.read();

                qDebug() << "read frame";
            }
            if (!writer->write(media.image())) {
                p->d.error = core::Error("mediaprocessor",
                                         QString("could not write frame for file: %1").arg(file.filename(frame)));
                qWarning() << "warning: " << p->d.error.message();
                return false;
            }
            progress_changed(next, timerange);
        }
        return true;
    }
    else {
        p->d.error = core::Error("mediaprocessor",
                                 QString("could not find plugin for extension: %1").arg(file.extension()));
        qWarning() << "warning: " << p->d.error.message();
        return false;
    }
}

core::Error
MediaProcessor::error() const
{
    return p->d.error;
}

bool
MediaProcessor::is_valid() const
{
    return true;
}

void
MediaProcessor::reset()
{
    p.reset(new MediaProcessorPrivate());
}

}  // namespace av
