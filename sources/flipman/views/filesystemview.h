// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QScopedPointer>
#include <QWidget>

namespace flipman {

class Session;
class FileSystemViewPrivate;

/**
 * @brief
 * Session-backed filesystem panel. Owns UI resources, not the session.
 */
class FileSystemView : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates the filesystem panel and connects it to the global Session.
     */
    explicit FileSystemView(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~FileSystemView() override;

private:
    /**
     * @brief
     * Connects this panel to application-owned session signals; called once after UI setup.
     */
    void bindSession();

    QScopedPointer<FileSystemViewPrivate> p;
};

}  // namespace flipman
