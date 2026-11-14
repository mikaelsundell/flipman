// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QScopedPointer>
#include <QWidget>

namespace flipman {

class Session;
class DisplayViewPrivate;

/**
 * @brief
 * Session-backed display panel. Owns UI resources, not the session.
 */
class DisplayView : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates the display panel and connects it to the global Session.
     */
    explicit DisplayView(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~DisplayView() override;

private:
    /**
     * @brief
     * Connects this panel to application-owned session signals; called once after UI setup.
     */
    void bindSession();

    QScopedPointer<DisplayViewPrivate> p;
};

}  // namespace flipman
