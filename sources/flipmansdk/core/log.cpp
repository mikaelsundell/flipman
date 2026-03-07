// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/core/log.h>

namespace flipman::sdk::core {

QTextStream&
logOut()
{
    static QTextStream stream(stdout);
    return stream;
}

QTextStream&
logErr()
{
    static QTextStream stream(stderr);
    return stream;
}

}  // namespace flipman::sdk::core
