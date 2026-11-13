// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <QString>
#include <QVariant>
#include <memory>
#include <stdexcept>

namespace nb = nanobind;
using namespace nb::literals;

namespace nanobind::detail {

template<> struct type_caster<QString> {
    NB_TYPE_CASTER(QString, const_name("str"))

    bool from_python(handle source, uint8_t, cleanup_list*) noexcept
    {
        if (!PyUnicode_Check(source.ptr()))
            return false;

        Py_ssize_t length = 0;
        const char* utf8 = PyUnicode_AsUTF8AndSize(source.ptr(), &length);
        if (!utf8)
            return false;

        value = QString::fromUtf8(utf8, length);
        return true;
    }

    static handle from_cpp(const QString& source, rv_policy, cleanup_list*) noexcept
    {
        const QByteArray utf8 = source.toUtf8();
        return PyUnicode_DecodeUTF8(utf8.constData(), utf8.size(), "strict");
    }
};

}  // namespace nanobind::detail

namespace flipman::sdk::python {

QVariant
toVariant(nb::handle value, unsigned depth = 0);
void
bindAv(nb::module_& module);
void
bindCore(nb::module_& module);
void
bindMedia(nb::module_& module);
void
bindRender(nb::module_& module);

template<typename T>
nb::list
list(const T& values)
{
    nb::list result;
    for (const auto& value : values)
        result.append(nb::cast(value, nb::rv_policy::copy));
    return result;
}

}  // namespace flipman::sdk::python

namespace flipman::sdk::python {

class RuntimePrivate;

/**
 * @brief Owns or borrows the SDK Qt application on the process main thread.
 *
 * Instances and their dependent objects must be used and released on that
 * thread. Multiple Runtime objects share an owned application. An application's
 * existing SDK application is borrowed and must outlive these wrappers.
 */
class Runtime {
public:
    Runtime();
    ~Runtime();
    void check() const;
    void processEvents();

private:
    std::shared_ptr<RuntimePrivate> p;
};

}  // namespace flipman::sdk::python
