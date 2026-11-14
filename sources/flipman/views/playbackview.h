// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QScopedPointer>
#include <QWidget>

namespace flipman {

class Session;
class PlaybackViewPrivate;

/**
 * @brief
 * Session-backed playback panel. Owns UI resources, not the session.
 */
class PlaybackView : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates the playback panel and connects it to the global Session.
     */
    explicit PlaybackView(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~PlaybackView() override;

private:
    /**
     * @brief
     * Connects this panel to application-owned session signals; called once after UI setup.
     */
    void bindSession();

    QScopedPointer<PlaybackViewPrivate> p;
};

}  // namespace flipman
