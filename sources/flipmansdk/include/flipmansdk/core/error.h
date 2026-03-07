// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk/flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QString>

namespace flipman::sdk::core {

class ErrorPrivate;

/**
 * @class Error
 * @brief Explicitly shared error object.
 *
 * Encapsulates domain, message, and error code.
 */
class FLIPMANSDK_EXPORT Error {
public:
    /**
     * @brief Constructs a null Error.
     */
    Error();

    /**
     * @brief Constructs an Error with details.
     *
     * @param domain Error category.
     * @param message Human-readable description.
     * @param code Optional numeric code.
     */
    explicit Error(const QString& domain, const QString& message, int code = 0);

    /**
     * @brief Copy constructor.
     */
    Error(const Error& other);

    /**
     * @brief Destroys the Error.
     */
    ~Error();

    /**
     * @brief Returns the error domain.
     */
    QString domain() const;

    /**
     * @brief Returns the error message.
     */
    QString message() const;

    /**
     * @brief Returns the error code.
     */
    int code() const;

    /**
     * @brief Returns true if the error is valid.
     */
    bool isValid() const;

    /**
     * @brief Alias for isValid().
     */
    bool hasError() const;

    /**
     * @brief Resets the Error to a null state.
     */
    void reset();

    /**
     * @brief Sets error details.
     */
    void setError(const QString& domain, const QString& message, int code);

    /** @name Operators */
    ///@{

    /**
     * @brief Assignment operator. Performs a shallow copy.
     */
    Error& operator=(const Error& other);

    /**
     * @brief Conversion operator returning the error message.
     */
    operator QString() const;

    ///@}

private:
    QExplicitlySharedDataPointer<ErrorPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Error)
