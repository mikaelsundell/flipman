// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/av/timeline.h>
#include <flipmansdk/core/metadata.h>
#include <flipmansdk/render/renderengine.h>
#include <QObject>
#include <QScopedPointer>

namespace flipman {

class CommandStack;
class SessionPrivate;

/**
 * @brief
 * Application document and playback controller. Owns its timeline and renderer.
 *
 * Use on the application thread. Decode work is serialized on a dedicated
 * worker; queued results retain native image storage without pixel conversion.
 * Opening a file creates one timeline/track/clip after first-frame validation.
 */
class Session : public QObject {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates the document controller, renderer, command history and serialized decoder worker.
     */
    explicit Session(QObject* parent = nullptr);

    /**
     * @brief
     * Retires queued decoder work and drops media handles before stopping the worker.
     */
    ~Session() override;

    /**
     * @brief
     * Returns the borrowed session-owned composition model; its identity changes on document replacement.
     */
    sdk::av::Timeline* timeline() const;

    /**
     * @brief
     * Returns the session-owned GPU renderer; callers must not delete it.
     */
    sdk::render::RenderEngine* renderEngine() const;

    /**
     * @brief
     * Returns session-owned undo/redo history; playback ticks do not create commands.
     */
    CommandStack* commands() const;

    /**
     * @brief
     * Returns the absolute filename of the accepted document, or empty when closed.
     */
    QString filename() const;

    /**
     * @brief
     * Returns source title metadata, falling back to the source filename.
     */
    QString title() const;

    /**
     * @brief
     * Returns the most recent load, playback or failure status message.
     */
    QString status() const;

    /**
     * @brief
     * Returns a shared value snapshot of the accepted source metadata.
     */
    sdk::core::MetaData metadata() const;

    /**
     * @brief
     * Returns shared native storage for the last presented image; performs no pixel conversion.
     */
    sdk::core::ImageBuffer image() const;

    /**
     * @brief
     * Returns whether the session has an accepted source and first decoded image.
     */
    bool isLoaded() const;

    /**
     * @brief
     * Returns whether a file-open request is pending on the decoder worker.
     */
    bool isLoading() const;

    /**
     * @brief
     * Returns whether playback waits for each image rather than following real-time deadlines.
     */
    bool everyFrame() const;

    /**
     * @brief
     * Returns timeline frame count, or zero without an accepted source.
     */
    qint64 frameCount() const;

    /**
     * @brief
     * Returns the zero-based frame whose image is currently submitted to the renderer.
     */
    qint64 presentedFrame() const;

    /**
     * @brief
     * Returns the explicit input encoding, empty for Auto or Raw for bypass.
     */
    QString inputColorSpace() const;

    /**
     * @brief
     * Returns the input encoding resolved from source tags and supported format defaults.
     */
    QString autoInputColorSpace() const;

    /**
     * @brief
     * Returns requested color settings, retaining user bypass selections.
     */
    sdk::render::ColorSettings colorSettings() const;

    /**
     * @brief
     * Returns the independent native surface tag used for macOS ColorSync.
     */
    sdk::render::DisplayTransform displayTransform() const;

    /**
     * @brief
     * Returns the selected canvas size; an empty size follows the source display window.
     */
    QSize projectResolution() const;

    /**
     * @brief
     * Validates input and working/output transforms, then updates cached GPU settings. Returns success.
     */
    bool setColorSettings(const sdk::render::ColorSettings& settings, const QString& input);

    /**
     * @brief
     * Changes the native surface descriptor without converting image pixels. Returns success.
     */
    bool setDisplayTransform(const sdk::render::DisplayTransform& transform);

    /**
     * @brief
     * Changes canvas size, or selects source resolution for an empty size.
     */
    void setProjectResolution(const QSize& size);

    /**
     * @brief
     * Changes the timeline canvas background when the color is valid.
     */
    void setBackground(const QColor& color);

    /**
     * @brief
     * Pauses playback and seeks to a valid zero-based frame; returns whether the request was accepted.
     */
    bool setFrame(qint64 frame);

    /**
     * @brief
     * Pauses playback and changes rational FPS while retaining frame indices. Returns success.
     */
    bool setFps(const sdk::av::Fps& fps);

    /**
     * @brief
     * Selects sequential image presentation instead of deadline-based frame skipping.
     */
    void setEveryFrame(bool enabled);

    /**
     * @brief
     * Moves the inclusive in point to the current timeline frame when loaded.
     */
    void setInPoint();

    /**
     * @brief
     * Moves the exclusive out boundary to the frame after the current frame when loaded.
     */
    void setOutPoint();

public Q_SLOTS:
    /**
     * @brief
     * Requests asynchronous file opening through the session; does not write source files.
     */
    void openFile(const QString& filename);

    /**
     * @brief
     * Cancels pending presentation, clears history and installs an empty timeline.
     */
    void closeFile();

    /**
     * @brief
     * Starts or stops playback of a ready source, restarting at the in point after the end.
     */
    void togglePlaying();

Q_SIGNALS:
    /**
     * @brief
     * Reports playback time, rate, range, loop or transport state changes.
     */
    void playbackChanged();

    /**
     * @brief
     * Reports any observable session change; prefer specific signals for UI work.
     */
    void changed();

    /**
     * @brief
     * Reports replacement of the timeline object; use timeline() to obtain the new borrowed model.
     */
    void timelineChanged();

    /**
     * @brief
     * Reports acceptance or removal of a source and its metadata.
     */
    void mediaChanged();

    /**
     * @brief
     * Reports a newly submitted image or cleared viewport; decode completion may follow a prior seek.
     */
    void frameChanged();

    /**
     * @brief
     * Reports accepted input, working or output transform settings.
     */
    void colorChanged();

    /**
     * @brief
     * Reports a surface tag, canvas resolution or background change.
     */
    void displayChanged();

    /**
     * @brief
     * Reports a new load, playback or failure message.
     */
    void statusChanged(const QString& text);

    /**
     * @brief
     * Reports entry into or exit from asynchronous file loading.
     */
    void loadingChanged(bool loading);

    /**
     * @brief
     * Reports the filename after first-frame validation and timeline installation.
     */
    void opened(const QString& filename);

    /**
     * @brief
     * Reports a file-open failure while retaining the previous accepted document.
     */
    void openFailed(const QString& message);

private:
    Q_DISABLE_COPY_MOVE(Session)
    QScopedPointer<SessionPrivate> p;
};

}  // namespace flipman
