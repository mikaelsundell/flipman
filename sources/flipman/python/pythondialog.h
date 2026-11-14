// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QDialog>
#include <QScopedPointer>

namespace flipman {

class PythonDialogPrivate;

/**
 * @brief
 * Persistent editor/output dialog for the application-owned interpreter.
 */
class PythonDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief
     * Creates a persistent editor/output dialog using the global application interpreter.
     */
    explicit PythonDialog(QWidget* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~PythonDialog() override;

private:
    QScopedPointer<PythonDialogPrivate> p;
};

}  // namespace flipman
