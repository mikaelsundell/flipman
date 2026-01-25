// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QHash>
#include <QVariant>
#include <core/parameters.h>

namespace core {
class ParametersPrivate : public QSharedData {
public:
    struct Data {
        QMap<QString, QVariant> data;
    };
    Data d;
};

Parameters::Parameters()
    : p(new ParametersPrivate())
{}

Parameters::Parameters(const Parameters& other)
    : p(other.p)
{}

Parameters::~Parameters() {}

QList<QString>
Parameters::keys() const
{
    return p->d.data.keys();
}

bool
Parameters::is_valid() const
{
    return p->d.data.size();
}

QVariant
Parameters::value(const QString& key) const
{
    return p->d.data.value(key);
}

void
Parameters::insert(const QString& key, const QVariant& value)
{
    p.detach();
    p->d.data.insert(key, value);
}

void
Parameters::remove(const QString& key)
{
    p.detach();
    p->d.data.remove(key);
}

void
Parameters::reset()
{
    p.reset(new ParametersPrivate());
}


QVariant&
Parameters::operator[](const QString& key)
{
    p.detach();
    return p->d.data[key];
}

Parameters&
Parameters::operator=(const Parameters& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

QString
Parameters::convert(Parameters::Key key)
{
    switch (key) {
    case Title: return "Title";
    case Author: return "Author";
    case Command: return "Command";
    case Description: return "Description";
    default: return QString();
    }
}
}  // namespace core
