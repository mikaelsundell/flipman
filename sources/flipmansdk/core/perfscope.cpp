// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell
// Adapted from https://github.com/mikaelsundell/stageviz

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <chrono>
#include <flipmansdk/core/perfscope.h>

namespace flipman::sdk::core {
PerfScope::PerfScope(const char* name)
    : d { name, enabled() ? now() : 0.0, enabled() ? QDateTime::currentMSecsSinceEpoch() : 0 }
{}

PerfScope::~PerfScope()
{
    if (d.start == 0.0)
        return;
    const double finish = now();
    const QJsonObject event { { "phase", QString::fromLatin1(d.name) },
                              { "start_ms", d.start },
                              { "wall_start_ms", d.wallStart },
                              { "duration_ms", finish - d.start },
                              { "thread",
                                QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16) } };
    static QMutex mutex;
    QMutexLocker lock(&mutex);
    QFile output(path());
    if (output.open(QIODevice::WriteOnly | QIODevice::Append))
        output.write(QJsonDocument(event).toJson(QJsonDocument::Compact) + '\n');
}

bool
PerfScope::enabled()
{
    return !path().isEmpty();
}
const QString&
PerfScope::path()
{
    static const QString value = qEnvironmentVariable("FLIPMAN_PERF_LOG");
    return value;
}
double
PerfScope::now()
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
}  // namespace flipman::sdk::core
