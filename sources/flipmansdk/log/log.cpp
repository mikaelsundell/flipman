// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024 - present Mikael Sundell
// https://github.com/mikaelsundell/flipman

#include <flipmansdk/log/log.h>

namespace flipman::sdk::log {

QTextStream&
out()
{
    static QTextStream out(stdout);
    return out;
}

QTextStream&
err()
{
    static QTextStream err(stderr);
    return err;
}

}  // namespace flipman::sdk::log
