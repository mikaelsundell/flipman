// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QVariant>

namespace flipman {

class SettingsPrivate;

/**
 * @brief
 * Application-owned preferences. Use on the GUI thread.
 *
 * An isolated run uses one explicit INI file with fallbacks disabled, preventing
 * reads or writes to normal preferences. The application configures this before
 * creating its session or views.
 */
class Settings : public QObject {
    Q_OBJECT
public:
    /**
     * @brief
     * Opens normal preferences, or an isolated INI file in the supplied directory.
     */
    explicit Settings(const QString& directory = {}, QObject* parent = nullptr);

    /**
     * @brief
     * Releases owned resources; borrowed application subsystems remain alive.
     */
    ~Settings() override;

    /**
     * @brief
     * Reads a preference, returning the supplied default when absent.
     */
    QVariant value(const QString& key, const QVariant& defaultValue = {}) const;

    /**
     * @brief
     * Writes a preference and notifies observers when its value changes.
     */
    void setValue(const QString& key, const QVariant& value);

    /**
     * @brief
     * Returns the underlying settings filename for diagnostics and clean runs.
     */
    QString filename() const;

    /**
     * @brief
     * Returns whether user and system preference fallbacks are disabled.
     */
    bool isIsolated() const;

    /**
     * @brief
     * Flushes pending preference writes to disk.
     */
    void sync();

Q_SIGNALS:
    /**
     * @brief
     * Reports a changed preference key and its new value.
     */
    void changed(const QString& key, const QVariant& value);

private:
    Q_DISABLE_COPY_MOVE(Settings)
    QScopedPointer<SettingsPrivate> p;
};

}  // namespace flipman
