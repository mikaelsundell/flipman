// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <flipmansdk.h>

#include <QExplicitlySharedDataPointer>
#include <QMetaType>
#include <QString>

namespace flipman::sdk::core {

class ErrorPrivate;

/**
 * @class Error
 * @brief Represents a system or application error with domain-specific context.
 * * Uses explicit data sharing for efficient passing. An Error consists of a
 * domain (category), a human-readable message, and a numeric status code.
 */
class FLIPMANSDK_EXPORT Error {
public:
    /// Constructs a null error with no message or domain.
    Error();

    /**
     * @brief Constructs an error with specific details.
     * @param domain The category of the error (e.g., "IO", "Network", "Internal").
     * @param message A human-readable description of the error.
     * @param code A specific error code for programmatic handling.
     */
    Error(const QString& domain, const QString& message, int code = 0);

    /// Copy constructor (shallow copy).
    Error(const Error& other);

    virtual ~Error();

    /// Returns the error domain/category.
    QString domain() const;

    /// Returns the human-readable error message.
    QString message() const;

    /// Returns the numeric error code.
    int code() const;

    /// Returns true if an error is present (alias for isValid).
    bool hasError() const;

    /// Returns true if the error object contains valid error information.
    bool isValid() const;

    /// Resets the error object to a null state.
    void reset();

    /// Sets new error details, detaching from shared data if necessary.
    void setError(const QString& domain, const QString& message, int code);

    Error& operator=(const Error& other);

    /// Convenience operator to return the error message as a QString.
    operator QString() const;

private:
    QExplicitlySharedDataPointer<ErrorPrivate> p;
};

}  // namespace flipman::sdk::core

/**
 * @note Registering the widget type for use in signals/slots and QVariant.
 */
Q_DECLARE_METATYPE(flipman::sdk::core::Error)
