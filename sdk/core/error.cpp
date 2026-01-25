// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <QDebug>
#include <core/error.h>

namespace core {
class ErrorPrivate : public QSharedData {
public:
    struct Data {
        QString domain;
        QString message;
        int code = 0;
    };
    Data d;
};

Error::Error()
    : p(new ErrorPrivate())
{}

Error::Error(const QString& domain, const QString& message, int code)
    : p(new ErrorPrivate())
{
    p->d.domain = domain;
    p->d.message = message;
    p->d.code = code;
}

Error::Error(const Error& other)
    : p(other.p)
{}

Error::~Error() {}

bool
Error::is_valid() const
{
    return !has_error();
}

QString
Error::domain() const
{
    return p->d.domain;
}

QString
Error::message() const
{
    return p->d.message;
}

int
Error::code() const
{
    return p->d.code;
}

bool
Error::has_error() const
{
    return !p->d.message.isEmpty();
}

void
Error::reset()
{
    p.reset(new ErrorPrivate());
}

void
Error::set_error(const QString& domain, const QString& message, int code)
{
    if (p->ref.loadRelaxed() > 1) {
        p.detach();
    }
    p->d.domain = domain;
    p->d.message = message;
    p->d.code = code;
}

Error&
Error::operator=(const Error& other)
{
    if (this != &other) {
        p = other.p;
    }
    return *this;
}

Error::operator QString() const { return QString("%1 - %2 (%3)").arg(p->d.domain).arg(p->d.message).arg(p->d.code); }
}  // namespace core
