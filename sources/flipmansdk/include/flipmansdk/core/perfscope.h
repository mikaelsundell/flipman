// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 - present Mikael Sundell
// Adapted from https://github.com/mikaelsundell/stageviz

#pragma once

#include <QString>
#include <flipmansdk/flipmansdk.h>

namespace flipman::sdk::core {

/**
 * @brief Appends optional JSONL wall-clock phase timings.
 *
 * Set FLIPMAN_PERF_LOG before first use. The destination is cached process-wide;
 * empty or unset disables clocks and file I/O. Use a static string literal for
 * the name. Writes are synchronous and intended for bounded diagnostic runs.
 * These are inclusive host timings, not GPU hardware measurements.
 */
class FLIPMANSDK_EXPORT PerfScope final {
public:
    /**
     * @brief Starts a named phase if performance logging is enabled.
     */
    explicit PerfScope(const char* name);

    /**
     * @brief Appends the completed phase to the configured destination.
     */
    ~PerfScope();

    /**
     * @brief Returns whether a nonempty path was configured at first use.
     */
    static bool enabled();

private:
    static const QString& path();
    static double now();
    struct Data {
        const char* name;
        double start;
        qint64 wallStart;
    } d;
    Q_DISABLE_COPY_MOVE(PerfScope)
};

}  // namespace flipman::sdk::core
