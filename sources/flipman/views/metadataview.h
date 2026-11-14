// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QScopedPointer>
#include <QWidget>

namespace flipman {

class Session;
class MetadataViewPrivate;

/**
 * @brief
 * Session-backed metadata panel. Owns UI resources, not the session.
 */
class MetadataView : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates the metadata panel and connects it to the global Session.
     */
    explicit MetadataView(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~MetadataView() override;

private:
    /**
     * @brief
     * Connects this panel to application-owned session signals; called once after UI setup.
     */
    void bindSession();

    QScopedPointer<MetadataViewPrivate> p;
};

}  // namespace flipman
