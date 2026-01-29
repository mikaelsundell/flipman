// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QVariant>

namespace flipman::sdk::core {

class ParametersPrivate;

/**
 * @class Parameters
 * @brief Container for key-value metadata using explicit sharing.
 * * Provides a flexible way to store and retrieve named properties using QVariant,
 * commonly used for media metadata or configuration sets.
 */
class Parameters {
public:
    /**
     * @enum Key
     * @brief Standard metadata keys for common identification tasks.
     */
    enum Key { Title, Author, Command, Description };

    /// Constructs an empty parameters container.
    Parameters();

    /// Copy constructor (shallow copy).
    Parameters(const Parameters& other);

    virtual ~Parameters();

    /// Returns true if the container has been initialized and is not null.
    bool isValid() const;

    /// Returns a list of all keys currently stored.
    QList<QString> keys() const;

    /// Returns the value associated with the key, or an invalid QVariant if not found.
    QVariant value(const QString& key) const;

    /// Clears all keys and values.
    void reset();

    /// Inserts or updates a key-value pair.
    void insert(const QString& key, const QVariant& value);

    /// Removes the specified key from the container.
    void remove(const QString& key);

    /// Provides access to values via the subscript operator.
    QVariant& operator[](const QString& key);

    Parameters& operator=(const Parameters& other);

    /**
     * @brief Utility to convert a Key enum to its string representation.
     * @param key The metadata key to convert.
     * @return The corresponding QString.
     */
    static QString convert(Parameters::Key key);

private:
    QExplicitlySharedDataPointer<ParametersPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Parameters)
