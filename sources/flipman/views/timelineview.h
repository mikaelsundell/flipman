// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QScopedPointer>
#include <QWidget>

namespace flipman {

class Session;
class TimelineViewPrivate;

/**
 * @brief
 * Session-backed timeline panel. Owns UI resources, not the session.
 */
class TimelineView : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates the timeline panel and connects it to the global Session.
     */
    explicit TimelineView(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~TimelineView() override;

private:
    /**
     * @brief
     * Connects this panel to application-owned session signals; called once after UI setup.
     */
    void bindSession();

    QScopedPointer<TimelineViewPrivate> p;
};

}  // namespace flipman
