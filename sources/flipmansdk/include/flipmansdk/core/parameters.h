// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QMetaType>
#include <QVariant>

namespace flipman::sdk::core {

class ParametersPrivate;

/**
 * @class Parameters
 * @brief Container for key-value metadata using explicit sharing.
 *
 * Provides a flexible way to store and retrieve named properties using QVariant,
 * commonly used for media metadata, filter settings, or configuration sets.
 */
class FLIPMANSDK_EXPORT Parameters {
public:
    /**
     * @enum Key
     * @brief Standard metadata keys for common identification tasks.
     */
    enum Key { Title, Author, Command, Description };

    /**
     * @brief Constructs an empty parameters container.
     */
    Parameters();

    /**
     * @brief Copy constructor. Performs a shallow copy of the shared data.
     */
    Parameters(const Parameters& other);

    /**
     * @brief Destroys the parameters.
     * @note Required for the PIMPL pattern to safely delete ParametersPrivate.
     */
    ~Parameters();

    /**
     * @brief Returns true if the container has been initialized and is not null.
     */
    bool isValid() const;

    /**
     * @brief Returns a list of all keys currently stored.
     */
    QList<QString> keys() const;

    /**
     * @brief Returns the value associated with the key.
     * @param key The string identifier for the parameter.
     * @return The value as a QVariant, or an invalid QVariant if not found.
     */
    QVariant value(const QString& key) const;

    /**
     * @brief Clears all keys and values.
     */
    void reset();

    /**
     * @brief Inserts or updates a key-value pair.
     * @param key The string identifier.
     * @param value The value to store.
     */
    void insert(const QString& key, const QVariant& value);

    /**
     * @brief Removes the specified key from the container.
     * @param key The string identifier to remove.
     */
    void remove(const QString& key);

    /** @name Operators */
    ///@{
    /**
     * @brief Provides access to values via the subscript operator.
     * @note This operation will trigger a detach if the data is shared.
     */
    QVariant& operator[](const QString& key);

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    Parameters& operator=(const Parameters& other);
    ///@}

    /**
     * @brief Utility to convert a Key enum to its string representation.
     * @param key The metadata key to convert.
     * @return The corresponding QString.
     */
    static QString convert(Parameters::Key key);

private:
    QExplicitlySharedDataPointer<ParametersPrivate> p;  ///< Private implementation.
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Parameters)
