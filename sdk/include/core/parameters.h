// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#pragma once

#include <core/object.h>

#include <QExplicitlySharedDataPointer>

namespace core {
class ParametersPrivate;
class Parameters : public Object {
public:
    enum Key { Title, Author, Command, Description };
    Parameters();
    Parameters(const Parameters& other);
    virtual ~Parameters();
    bool is_valid() const;
    QList<QString> keys() const;
    QVariant value(const QString& key) const;
    void reset();

    void insert(const QString& key, const QVariant& value);
    void remove(const QString& key);

    QVariant& operator[](const QString& key);
    Parameters& operator=(const Parameters& other);

    static QString convert(Parameters::Key key);

private:
    QExplicitlySharedDataPointer<ParametersPrivate> p;
};
}  // namespace core
