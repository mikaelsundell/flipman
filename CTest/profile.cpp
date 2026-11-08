// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <flipmansdk/core/log.h>
#include <flipmansdk/core/perfscope.h>
#include <flipmansdk/core/profile.h>
#include <thread>
#include <vector>

namespace {
int messages = 0;
void
capture(QtMsgType, const QMessageLogContext&, const QString&)
{
    ++messages;
}
bool
recording(const QString& directory)
{
    namespace profile = flipman::sdk::profile;
    profile::setMode(profile::Mode::Off);
    profile::reset();
    static profile::Site parent { "test", "parent" };
    static profile::Site child { "test", "child" };
    {
        profile::Scope scope(parent);
    }
    if (parent.calls != 0)
        return false;
    const auto first = profile::beginRecording("first", (directory + "/first.trace").toStdString());
    const auto second = profile::beginRecording("second", (directory + "/second.trace").toStdString());
    if (!first || !second || !profile::recordingActive(first))
        return false;
    {
        profile::Scope scope(parent);
        profile::Scope nested(child);
    }
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i)
        workers.emplace_back([] {
            profile::Scope scope(child);
            profile::counter("frames", 1);
        });
    for (auto& worker : workers)
        worker.join();
    profile::event("test", "details", "line one\nline two");
    profile::endRecording(first);
    profile::endRecording(second);
    if (profile::recordingActive(first) || parent.calls != 1 || child.calls != 5
        || parent.inclusiveNs < parent.exclusiveNs)
        return false;
    for (const auto& name : { "/first.trace", "/second.trace" }) {
        QFile input(directory + name);
        if (!input.open(QIODevice::ReadOnly))
            return false;
        const auto text = input.readAll();
        if (text.count("BEGIN | test::child") != 5 || text.count("END | test::child") != 5
            || text.count("COUNT | frames") != 4 || !text.contains("line one line two"))
            return false;
    }
    if (profile::summary().find("test::parent") == std::string::npos)
        return false;
    if (!profile::writeSummary((directory + "/summary.txt").toStdString()))
        return false;
    if (profile::beginRecording("invalid name", (directory + "/invalid.trace").toStdString()) != 0)
        return false;
    profile::setMode(profile::Mode::Trace);
    profile::setSlowThresholdUs(0);
    profile::setTraceCapacity(2);
    for (int i = 0; i < 3; ++i) {
        profile::Scope scope(parent);
    }
    if (profile::summary().find("ring capacity 2") == std::string::npos)
        return false;
    profile::reset();
    profile::setMode(profile::Mode::Off);
    return parent.calls == 0 && child.calls == 0;
}
bool
phaseLog(const QString& mode, const QString& directory)
{
    using flipman::sdk::core::PerfScope;
    const QString path = directory + "/phases.jsonl";
    if (mode == "unset")
        qunsetenv("FLIPMAN_PERF_LOG");
    else
        qputenv("FLIPMAN_PERF_LOG", mode == "empty" ? QByteArray() : path.toUtf8());
    const bool expected = mode == "enabled";
    if (PerfScope::enabled() != expected)
        return false;
    {
        PerfScope parent("test.parent");
        {
            PerfScope child("test.child");
        }
    }
    qputenv("FLIPMAN_PERF_LOG", (directory + "/other.jsonl").toUtf8());
    {
        PerfScope cached("test.cached");
    }
    if (!expected)
        return !QFile::exists(path) && !QFile::exists(directory + "/other.jsonl");
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i)
        workers.emplace_back([] { PerfScope scope("test.worker"); });
    for (auto& worker : workers)
        worker.join();
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly))
        return false;
    int count = 0;
    while (!input.atEnd()) {
        const auto document = QJsonDocument::fromJson(input.readLine());
        const auto event = document.object();
        if (document.isNull() || event["duration_ms"].toDouble(-1) < 0 || event["phase"].toString().isEmpty()
            || event["thread"].toString().isEmpty() || event["wall_start_ms"].toDouble() <= 0)
            return false;
        ++count;
    }
    return count == 7 && !QFile::exists(directory + "/other.jsonl");
}
bool
logging()
{
    using namespace flipman::sdk::core;
    auto old = qInstallMessageHandler(capture);
    qCDebug(mediaLog) << "disabled media debug";
    qCDebug(renderLog) << "disabled render debug";
    bool ok = messages == 0;
    qCWarning(mediaLog) << "visible warning";
    ok = ok && messages == 1;
    QLoggingCategory::setFilterRules("flipman.media.debug=true\nflipman.render.debug=true");
    qCDebug(mediaLog) << "enabled media debug";
    qCDebug(renderLog) << "enabled render debug";
    ok = ok && messages == 3;
    qInstallMessageHandler(old);
    return ok;
}
}  // namespace
int
main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    if (argc != 2 || !directory.isValid())
        return 2;
    const QString name = QString::fromLocal8Bit(argv[1]);
    if (name == "recording")
        return recording(directory.path()) ? 0 : 1;
    if (name == "logging")
        return logging() ? 0 : 1;
    if (name == "unset" || name == "empty" || name == "enabled")
        return phaseLog(name, directory.path()) ? 0 : 1;
    return 2;
}
