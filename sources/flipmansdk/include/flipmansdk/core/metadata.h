// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QMetaType>
#include <QStringList>
#include <QVariant>

namespace flipman::sdk::core {

class MetaDataPrivate;

/**
 * @class MetaData
 * @brief Explicitly shared namespaced key-value container.
 */
class FLIPMANSDK_EXPORT MetaData {
public:
    /**
     * @brief Metadata namespace.
     */
    enum class Group { Container, Video, Audio, Camera, Timecode, Production, Custom };

    /**
     * @brief Standard root keys.
     */
    enum Key { Title, Author, Command, Description };

    /**
     * @brief Constructs an empty MetaData container.
     */
    MetaData();

    /**
     * @brief Copy constructor.
     */
    MetaData(const MetaData& other);

    /**
     * @brief Destroys the MetaData container.
     */
    ~MetaData();

    /**
     * @brief Returns true if valid.
     */
    bool isValid() const;

    /**
     * @brief Returns groups containing values.
     */
    QList<Group> groups() const;

    /**
     * @brief Returns keys for a group.
     */
    QList<QString> keys(Group group) const;

    /**
     * @brief Returns value for root key.
     */
    QVariant value(const QString& key) const;

    /**
     * @brief Returns value for grouped key.
     */
    QVariant value(Group group, const QString& key) const;

    /**
     * @brief Clears all entries.
     */
    void reset();

    /**
     * @brief Inserts or updates root key.
     */
    void insert(const QString& key, const QVariant& value);

    /**
     * @brief Inserts or updates grouped key.
     */
    void insert(Group group, const QString& key, const QVariant& value);

    /**
     * @brief Removes root key.
     */
    void remove(const QString& key);

    /**
     * @brief Removes grouped key.
     */
    void remove(Group group, const QString& key);

    /** @name Operators */
    ///@{

    /**
     * @brief Subscript operator for root keys.
     *
     * Detaches if shared.
     */
    QVariant& operator[](const QString& key);

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    MetaData& operator=(const MetaData& other);

    ///@}

    /**
     * @brief Converts Key enum to string.
     */
    static QString convert(Key key);

    /**
     * @brief Converts Group enum to string.
     */
    static QString convert(Group group);

private:
    QExplicitlySharedDataPointer<MetaDataPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::MetaData)
