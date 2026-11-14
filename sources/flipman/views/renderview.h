// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <flipmansdk/widgets/viewer.h>
#include <QScopedPointer>
#include <QWidget>

namespace flipman {

class Session;
class RenderViewPrivate;

/**
 * @brief
 * Session-backed render panel. Owns UI resources, not the session.
 */
class RenderView : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates the render panel and connects it to the global Session.
     */
    explicit RenderView(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~RenderView() override;


    /**
     * @brief
     * Returns the owned SDK Metal viewport; its render engine is borrowed from Session.
     */
    sdk::widgets::Viewer* viewport() const;

    /**
     * @brief
     * Fits the timeline canvas to the available viewport without altering image pixels.
     */
    void fitToView();

private:
    /**
     * @brief
     * Connects this panel to application-owned session signals; called once after UI setup.
     */
    void bindSession();

    QScopedPointer<RenderViewPrivate> p;
};

}  // namespace flipman
