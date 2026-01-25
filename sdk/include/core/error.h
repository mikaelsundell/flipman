// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/object.h>

#include <QExplicitlySharedDataPointer>
#include <QString>

namespace core {
class ErrorPrivate;
class Error : public Object {
public:
    Error();
    Error(const QString& domain, const QString& message, int code = 0);
    Error(const Error& other);
    virtual ~Error();
    bool is_valid() const override;
    QString domain() const;
    QString message() const;
    int code() const;
    bool has_error() const;
    void reset() override;

    void set_error(const QString& domain, const QString& message, int code);

    Error& operator=(const Error& other);
    operator QString() const;

private:
    QExplicitlySharedDataPointer<ErrorPrivate> p;
};
}  // namespace core
