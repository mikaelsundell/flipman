// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#include "commandstack.h"
#include "session.h"

#include <flipmansdk/av/clip.h>
#include <flipmansdk/av/media.h>
#include <flipmansdk/core/file.h>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QFileInfo>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <memory>

namespace flipman {

namespace {

    struct DecodeResult {
        sdk::av::Media media;
        sdk::core::ImageBuffer image;
        sdk::core::MetaData metadata;
        sdk::av::TimeRange range;
        sdk::av::Time start;
        QString error;
        qint64 frame = 0;
        quint64 token = 0;
    };

    // This object and its reader are created, used and released on one worker.
    class DecodeWorker : public QObject {
    public:
        std::shared_ptr<DecodeResult> open(const QString& filename)
        {
            auto result = std::make_shared<DecodeResult>();
            sdk::av::Media candidate;
            if (!candidate.open(sdk::core::File(filename)) || !candidate.waitForOpened(5000)) {
                result->error = candidate.error().message();
                if (result->error.isEmpty())
                    result->error = "Media open failed or timed out";
                return result;
            }
            if (!candidate.read().isValid() || !candidate.image().isAllocated()) {
                result->error = "Could not decode the first image";
                return result;
            }
            result->token = ++d.serial;
            d.pending.insert(result->token, candidate);
            result->media = candidate;
            result->image = candidate.image();
            result->metadata = candidate.metaData();
            result->range = candidate.timeRange();
            result->start = candidate.start();
            return result;
        }

        std::shared_ptr<DecodeResult> read(qint64 frame)
        {
            auto result = std::make_shared<DecodeResult>();
            result->frame = frame;
            if (!d.media.isOpen()) {
                result->error = "No media is open";
                return result;
            }
            if (frame != d.nextFrame) {
                const auto range = d.media.timeRange();
                auto start = sdk::av::Time::fromFrames(range.start().frames() + frame, d.media.fps());
                auto duration = sdk::av::Time::fromFrames(std::max<qint64>(1, range.duration().frames() - frame),
                                                          d.media.fps());
                if (!d.media.seek(sdk::av::TimeRange(start, duration)).isValid()) {
                    result->error = "Could not seek to the requested frame";
                    return result;
                }
            }
            if (!d.media.read().isValid() || !d.media.image().isAllocated()) {
                result->error = d.media.error().message();
                if (result->error.isEmpty())
                    result->error = "Could not decode the requested frame";
                return result;
            }
            result->image = d.media.image();
            d.nextFrame = frame + 1;
            return result;
        }

        void commit(quint64 token)
        {
            d.media = d.pending.take(token);
            d.nextFrame = 1;
        }

        void discard(quint64 token) { d.pending.remove(token); }

        void close()
        {
            d.media.close();
            d.media.reset();

            // Pending opens retain their readers until the GUI discards their result.
            d.nextFrame = 0;
        }

    private:
        struct Data {
            sdk::av::Media media;
            QHash<quint64, sdk::av::Media> pending;
            quint64 serial = 0;
            qint64 nextFrame = 0;
        };
        Data d;
    };

    QString sourceEncoding(const DecodeResult& result, const QString& filename)
    {
        auto encoding = sdk::render::ColorPipeline::inputColorSpace(result.image);
        if (!encoding.isEmpty())
            return encoding;

        const auto suffix = QFileInfo(filename).suffix().toLower();
        if (suffix == "braw") {
            if (result.metadata.value(sdk::core::MetaData::Group::Video, "decodeColorScienceGen").toInt() == 5
                && result.metadata.value(sdk::core::MetaData::Group::Video, "decodeGamma").toString()
                       == "Blackmagic Design Film")
                return "BMDFilm WideGamut Gen5";

            return {};
        }
        if (suffix == "exr" || suffix == "hdr")
            return "Linear Rec.709 (sRGB)";

        return result.range.isValid() && result.range.duration().frames() > 1 ? "Rec.1886 Rec.709 - Display"
                                                                              : "sRGB - Display";
    }

}  // namespace

class SessionPrivate : public QObject {
public:
    void initialize();
    void installTimeline(sdk::av::Timeline* timeline);
    void requestFrame(qint64 frame);
    void present(const std::shared_ptr<DecodeResult>& result);
    void tick();
    void status(const QString& message);
    void applyResolution();

    struct Data {
        sdk::core::MetaData metadata;
        sdk::render::ImageLayer layer;
        sdk::render::ColorSettings color;
        sdk::render::DisplayTransform display { sdk::render::ColorSpace::Rec709,
                                                sdk::render::TransferFunction::Gamma24 };
        QString filename;
        QString title;
        QString status = "Drop a movie or image to begin";
        QString input;
        QString autoInput;
        QSize projectResolution;
        QSize sourceResolution;
        QThread decodeThread;
        QTimer playback;
        QElapsedTimer clock;
        QPointer<DecodeWorker> worker;
        QPointer<sdk::av::Timeline> timeline;
        QPointer<sdk::render::RenderEngine> renderer;
        QPointer<CommandStack> commands;
        quint64 generation = 0;
        qint64 requested = 0;
        qint64 presented = 0;
        qint64 clockStartFrame = 0;
        bool busy = false;
        bool loading = false;
        bool loaded = false;
        bool everyFrame = false;
        QPointer<Session> session;
    };
    Data d;
};

void
SessionPrivate::status(const QString& message)
{
    d.status = message;
    Q_EMIT d.session->statusChanged(message);
}

void
SessionPrivate::initialize()
{
    d.renderer = new sdk::render::RenderEngine(d.session);
    d.renderer->setBackground(Qt::black);
    d.color.enabled = true;
    d.renderer->setColorSettings(d.color);
    d.commands = new CommandStack(d.session);
    installTimeline(new sdk::av::Timeline(d.session));
    d.worker = new DecodeWorker;
    d.worker->moveToThread(&d.decodeThread);
    connect(&d.decodeThread, &QThread::finished, d.worker, &QObject::deleteLater);
    d.decodeThread.start();
    d.playback.setTimerType(Qt::PreciseTimer);
    connect(&d.playback, &QTimer::timeout, this, [this]() { tick(); });
}

void
SessionPrivate::installTimeline(sdk::av::Timeline* timeline)
{
    if (d.timeline)
        delete d.timeline;
    d.timeline = timeline;
    connect(timeline, &sdk::av::Timeline::timeChanged, d.session, &Session::playbackChanged);
    connect(timeline, &sdk::av::Timeline::fpsChanged, d.session, &Session::playbackChanged);
    connect(timeline, &sdk::av::Timeline::playChanged, d.session, &Session::playbackChanged);
    connect(timeline, &sdk::av::Timeline::loopChanged, d.session, &Session::playbackChanged);
    connect(timeline, &sdk::av::Timeline::everyFrameChanged, d.session, &Session::playbackChanged);
    connect(timeline, &sdk::av::Timeline::timerangeChanged, d.session, &Session::playbackChanged);
    connect(timeline, &sdk::av::Timeline::ioRangeChanged, d.session, &Session::playbackChanged);
    connect(timeline, &sdk::av::Timeline::timeChanged, this,
            [this](const sdk::av::Time& time) { requestFrame(time.frames()); });
    connect(timeline, &sdk::av::Timeline::playChanged, this, [this](bool playing) {
        if (playing) {
            d.clockStartFrame = d.timeline->time().frames();
            d.clock.restart();
            const int period = std::max(1, int(1000.0 / d.timeline->fps().real() / (d.everyFrame ? 1 : 2)));
            d.playback.start(period);
        }
        else
            d.playback.stop();
    });
    Q_EMIT d.session->timelineChanged();
}

void
SessionPrivate::applyResolution()
{
    const auto size = d.projectResolution.isEmpty() ? d.sourceResolution : d.projectResolution;
    if (!size.isEmpty()) {
        d.renderer->setResolution(size);
        d.timeline->setWidth(size.width());
        d.timeline->setHeight(size.height());
    }
}

void
SessionPrivate::present(const std::shared_ptr<DecodeResult>& result)
{
    if (!result->error.isEmpty()) {
        d.timeline->stop();
        status(result->error);
        return;
    }
    d.presented = result->frame;
    d.layer.setImage(result->image);
    d.layer.setInputColorSpace(d.input.isEmpty() ? d.autoInput : d.input);
    d.renderer->setImageLayers({ d.layer });
    Q_EMIT d.session->frameChanged();
}

void
SessionPrivate::requestFrame(qint64 frame)
{
    d.requested = frame;
    if (!d.loaded || d.loading || d.busy || frame == d.presented)
        return;

    d.busy = true;
    const auto generation = d.generation;
    auto* worker = d.worker.data();
    QMetaObject::invokeMethod(
        worker,
        [this, worker, generation, frame]() {
            const auto result = worker->read(frame);
            QMetaObject::invokeMethod(
                this,
                [this, result, generation]() {
                    if (generation != d.generation) {
                        auto* worker = d.worker.data();
                        QMetaObject::invokeMethod(
                            worker, [worker, token = result->token]() { worker->discard(token); },
                            Qt::QueuedConnection);
                        return;
                    }
                    d.busy = false;
                    present(result);
                    if (result->error.isEmpty() && d.requested != result->frame)
                        requestFrame(d.requested);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}

void
SessionPrivate::tick()
{
    if (!d.loaded || d.loading || d.busy || !d.timeline->isPlaying())
        return;

    const auto range = d.timeline->ioRange().isValid() ? d.timeline->ioRange() : d.timeline->timeRange();
    qint64 frame = d.everyFrame ? d.timeline->time().frames() + 1
                                : d.clockStartFrame + qint64(d.clock.nsecsElapsed() * d.timeline->fps().real() / 1e9);
    if (frame >= range.end().frames()) {
        if (!d.timeline->loop()) {
            d.timeline->seek(sdk::av::Time::fromFrames(range.end().frames() - 1, d.timeline->fps()));
            d.timeline->stop();
            return;
        }
        frame = range.start().frames();
        d.clockStartFrame = frame;
        d.clock.restart();
    }
    d.timeline->seek(sdk::av::Time::fromFrames(std::max(frame, range.start().frames()), d.timeline->fps()));
}

Session::Session(QObject* parent)
    : QObject(parent)
    , p(new SessionPrivate)
{
    p->d.session = this;
    p->initialize();
    connect(this, &Session::playbackChanged, this, &Session::changed);
    connect(this, &Session::timelineChanged, this, &Session::changed);
    connect(this, &Session::frameChanged, this, &Session::changed);
    connect(this, &Session::mediaChanged, this, &Session::changed);
    connect(this, &Session::colorChanged, this, &Session::changed);
    connect(this, &Session::displayChanged, this, &Session::changed);
    connect(this, &Session::statusChanged, this, &Session::changed);
    connect(this, &Session::loadingChanged, this, &Session::changed);
}

Session::~Session()
{
    p->d.timeline->stop();
    ++p->d.generation;

    // Drop GUI media handles before releasing the final reader on its worker.
    delete p->d.timeline;
    p->d.timeline = nullptr;
    auto* worker = p->d.worker.data();

    // Finish worker jobs, then drop queued GUI snapshots while their readers
    // are still retained on the worker. Its final handles are released there.
    QMetaObject::invokeMethod(worker, []() {}, Qt::BlockingQueuedConnection);
    QCoreApplication::removePostedEvents(p.data(), QEvent::MetaCall);
    QMetaObject::invokeMethod(worker, [worker]() { worker->close(); }, Qt::BlockingQueuedConnection);
    p->d.decodeThread.quit();
    p->d.decodeThread.wait();
}

sdk::av::Timeline*
Session::timeline() const
{
    return p->d.timeline;
}

sdk::render::RenderEngine*
Session::renderEngine() const
{
    return p->d.renderer;
}

CommandStack*
Session::commands() const
{
    return p->d.commands;
}

QString
Session::filename() const
{
    return p->d.filename;
}

QString
Session::title() const
{
    return p->d.title;
}

QString
Session::status() const
{
    return p->d.status;
}

sdk::core::MetaData
Session::metadata() const
{
    return p->d.metadata;
}

sdk::core::ImageBuffer
Session::image() const
{
    return p->d.layer.image();
}

bool
Session::isLoaded() const
{
    return p->d.loaded;
}

bool
Session::isLoading() const
{
    return p->d.loading;
}

bool
Session::everyFrame() const
{
    return p->d.everyFrame;
}

qint64
Session::frameCount() const
{
    return p->d.loaded ? p->d.timeline->timeRange().duration().frames() : 0;
}

qint64
Session::presentedFrame() const
{
    return p->d.presented;
}

QString
Session::inputColorSpace() const
{
    return p->d.input;
}

QString
Session::autoInputColorSpace() const
{
    return p->d.autoInput;
}

sdk::render::ColorSettings
Session::colorSettings() const
{
    return p->d.color;
}

sdk::render::DisplayTransform
Session::displayTransform() const
{
    return p->d.display;
}

QSize
Session::projectResolution() const
{
    return p->d.projectResolution;
}

void
Session::openFile(const QString& filename)
{
    if (!QFileInfo(filename).isFile()) {
        p->status("File does not exist: " + filename);
        Q_EMIT openFailed(p->d.status);
        return;
    }
    p->d.timeline->stop();
    const auto generation = ++p->d.generation;
    p->d.loading = true;
    p->d.busy = false;
    Q_EMIT loadingChanged(true);
    p->status("Opening " + QFileInfo(filename).fileName());
    auto* worker = p->d.worker.data();
    auto* context = p.data();
    QMetaObject::invokeMethod(
        worker,
        [context, worker, filename, generation]() {
            const auto result = worker->open(filename);
            QMetaObject::invokeMethod(
                context,
                [context, result, filename, generation]() {
                    auto& d = context->d;
                    if (generation != d.generation) {
                        auto* worker = d.worker.data();
                        QMetaObject::invokeMethod(
                            worker, [worker, token = result->token]() { worker->discard(token); },
                            Qt::QueuedConnection);
                        return;
                    }
                    d.loading = false;
                    Q_EMIT d.session->loadingChanged(false);
                    if (!result->error.isEmpty()) {
                        context->status(result->error);
                        Q_EMIT d.session->openFailed(result->error);
                        context->requestFrame(d.timeline->time().frames());
                        return;
                    }
                    auto* timeline = new sdk::av::Timeline(d.session);
                    const auto fps = result->range.isValid() ? result->range.duration().fps() : sdk::av::Fps::fps24();
                    const auto duration
                        = sdk::av::Time::fromFrames(std::max<qint64>(1, result->range.duration().frames()), fps);
                    const sdk::av::TimeRange range(sdk::av::Time::zero(fps), duration);
                    timeline->setEveryFrame(d.everyFrame);
                    timeline->setTimeRange(range);
                    timeline->setIoRange(range);
                    timeline->setStartTime(result->start);
                    auto* track = new sdk::av::Track(timeline);
                    auto* clip = new sdk::av::Clip(track);
                    clip->setMedia(result->media);
                    clip->setName(QFileInfo(filename).fileName());
                    track->insertClip(clip, range);
                    timeline->insertTrack(track);
                    d.commands->clear();
                    d.filename = QFileInfo(filename).absoluteFilePath();
                    d.metadata = result->metadata;
                    d.title = d.metadata.value("title").toString().trimmed();
                    if (d.title.isEmpty())
                        d.title = d.metadata.value(sdk::core::MetaData::Group::Container, "title").toString().trimmed();
                    if (d.title.isEmpty())
                        d.title = QFileInfo(filename).fileName();
                    d.sourceResolution = result->image.displayWindow().size();
                    d.autoInput = sourceEncoding(*result, filename);
                    d.loaded = true;
                    d.requested = d.presented = 0;
                    context->installTimeline(timeline);
                    auto* worker = d.worker.data();
                    const auto token = result->token;
                    QMetaObject::invokeMethod(
                        worker, [worker, token]() { worker->commit(token); }, Qt::QueuedConnection);
                    context->applyResolution();
                    d.session->setColorSettings(d.color, d.input);
                    context->present(result);
                    context->status("Ready");
                    Q_EMIT d.session->mediaChanged();
                    Q_EMIT d.session->opened(d.filename);
                },
                Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}

void
Session::closeFile()
{
    p->d.timeline->stop();
    ++p->d.generation;
    p->d.loaded = p->d.loading = p->d.busy = false;
    p->d.commands->clear();
    p->installTimeline(new sdk::av::Timeline(this));
    p->d.autoInput.clear();
    p->d.sourceResolution = {};
    p->d.filename.clear();
    p->d.title.clear();
    p->d.metadata.reset();
    p->d.layer = {};
    p->d.renderer->setImageLayers({});
    auto* worker = p->d.worker.data();
    QMetaObject::invokeMethod(worker, [worker]() { worker->close(); }, Qt::QueuedConnection);
    p->status("Drop a movie or image to begin");
    Q_EMIT loadingChanged(false);
    Q_EMIT mediaChanged();
    Q_EMIT frameChanged();
}

bool
Session::setFrame(qint64 frame)
{
    if (!p->d.loaded || frame < 0 || frame >= frameCount())
        return false;

    p->d.timeline->stop();
    p->d.timeline->seek(sdk::av::Time::fromFrames(frame, p->d.timeline->fps()));
    return true;
}

bool
Session::setFps(const sdk::av::Fps& fps)
{
    if (!fps.isValid() || fps.real() <= 0 || fps.real() > 1000)
        return false;

    p->d.timeline->stop();
    p->d.timeline->setFps(fps);
    for (auto* track : p->d.timeline->tracks())
        for (auto* clip : track->clips())
            track->insertClip(clip, p->d.timeline->timeRange());
    return true;
}

void
Session::setEveryFrame(bool enabled)
{
    p->d.everyFrame = enabled;
    p->d.timeline->setEveryFrame(enabled);
}

void
Session::togglePlaying()
{
    if (!p->d.loaded)
        return;

    if (p->d.timeline->isPlaying())
        p->d.timeline->stop();
    else {
        const auto range = p->d.timeline->ioRange();
        if (p->d.timeline->time().frames() >= range.end().frames() - 1)
            p->d.timeline->seek(range.start());
        p->d.timeline->play();
    }
}

void
Session::setInPoint()
{
    if (!isLoaded())
        return;

    auto* t = timeline();
    const auto end = t->ioRange().end().frames();
    if (t->time().frames() < end)
        t->setIoRange({ t->time(), sdk::av::Time::fromFrames(end - t->time().frames(), t->fps()) });
}

void
Session::setOutPoint()
{
    if (!isLoaded())
        return;

    auto* t = timeline();
    const auto start = t->ioRange().start().frames();
    if (t->time().frames() >= start)
        t->setIoRange({ sdk::av::Time::fromFrames(start, t->fps()),
                        sdk::av::Time::fromFrames(t->time().frames() + 1 - start, t->fps()) });
}

bool
Session::setColorSettings(const sdk::render::ColorSettings& settings, const QString& input)
{
    try {
        if (!input.isEmpty() && input != "Raw" && !sdk::render::ColorPipeline::colorSpaces().contains(input))
            return false;

        auto resolved = settings;
        if (resolved.workingSpace == "Raw") {
            resolved.workingSpace = input.isEmpty() ? p->d.autoInput : input;
            if (resolved.workingSpace.isEmpty() || resolved.workingSpace == "Raw")
                resolved.workingSpace = "ACEScg";
            for (const auto& profile : sdk::render::ColorPipeline::colorProfiles())
                if (profile.name == resolved.workingSpace) {
                    resolved.workingSpace = sdk::render::ColorPipeline::combinedColorSpace(profile.gamut,
                                                                                           profile.encoding);
                    break;
                }
        }
        if (!p->d.renderer->setColorSettings(resolved)) {
            p->status(p->d.renderer->error().message());
            return false;
        }
        p->d.color = settings;
        p->d.input = input;
        p->d.layer.setInputColorSpace(input.isEmpty() ? p->d.autoInput : input);
        if (p->d.loaded)
            p->d.renderer->setImageLayers({ p->d.layer });
        Q_EMIT colorChanged();
        return true;
    } catch (const std::exception& error) {
        p->status(QString::fromUtf8(error.what()));
        return false;
    }
}

bool
Session::setDisplayTransform(const sdk::render::DisplayTransform& transform)
{
    p->d.display = transform;
    Q_EMIT displayChanged();
    return true;
}

void
Session::setProjectResolution(const QSize& size)
{
    if (size.isEmpty() || (size.width() > 0 && size.height() > 0)) {
        p->d.projectResolution = size;
        p->applyResolution();
        Q_EMIT displayChanged();
    }
}

void
Session::setBackground(const QColor& color)
{
    if (color.isValid()) {
        p->d.renderer->setBackground(color);
        Q_EMIT displayChanged();
    }
}

}  // namespace flipman
