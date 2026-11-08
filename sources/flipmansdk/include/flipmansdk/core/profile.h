// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025 - present Mikael Sundell
// Adapted from https://github.com/mikaelsundell/stageviz

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <flipmansdk/flipmansdk.h>
#include <string>

namespace flipman::sdk::profile {

/**
 * @brief Controls instrumentation outside explicitly recorded diagnostic sessions.
 */
enum class Mode : uint8_t {
    /**
     * @brief Measures only while an explicit recording is active.
     */
    Off,
    /**
     * @brief Accumulates inclusive and exclusive scope timings.
     */
    Aggregate,
    /**
     * @brief Also retains slow scopes in a bounded ring.
     */
    Trace
};

/**
 * @brief Static registration and accumulated timings for one instrumented source location.
 *
 * Construct sites with static storage and static category/name strings. They
 * remain registered for process lifetime; temporary sites are unsupported.
 */
struct FLIPMANSDK_EXPORT Site {
    /**
     * @brief Static category name.
     */
    const char* category;
    /**
     * @brief Static operation name.
     */
    const char* name;
    /**
     * @brief Completed scope count.
     */
    std::atomic<uint64_t> calls { 0 };
    /**
     * @brief Elapsed nanoseconds including nested scopes.
     */
    std::atomic<uint64_t> inclusiveNs { 0 };
    /**
     * @brief Elapsed nanoseconds excluding nested scopes.
     */
    std::atomic<uint64_t> exclusiveNs { 0 };
    /**
     * @brief Longest completed scope in nanoseconds.
     */
    std::atomic<uint64_t> maxNs { 0 };
    /**
     * @brief Registers a process-lifetime source location.
     */
    Site(const char* category, const char* name);
    Site(const Site&) = delete;
    Site& operator=(const Site&) = delete;
};

/**
 * @brief Sets instrumentation mode; explicit recordings remain independent.
 */
FLIPMANSDK_EXPORT void
setMode(Mode mode) noexcept;
/**
 * @brief Returns the current instrumentation mode, initially Off.
 */
FLIPMANSDK_EXPORT Mode
mode() noexcept;
/**
 * @brief Sets the slow-call threshold used by Trace mode.
 */
FLIPMANSDK_EXPORT void
setSlowThresholdUs(uint64_t microseconds) noexcept;
/**
 * @brief Resizes and clears the slow-call ring.
 */
FLIPMANSDK_EXPORT void
setTraceCapacity(size_t events);

/**
 * @brief Identifies a named recording; zero indicates failure.
 */
using RecordingId = uint64_t;

/**
 * @brief Starts an independently named diagnostic recording.
 *
 * Multiple recordings may overlap, and each receives events from all
 * instrumented threads. The function returns a nonzero handle on success, or
 * zero if the output file cannot be opened.
 *
 * Each recording writes BEGIN/END scope events and numeric counters. If no
 * filename or FLIPMAN_PROFILE_REPORT is supplied, output goes to stdout. The
 * resolved destination is printed to stderr when recording starts. A non-empty
 * filename is used as-is. File recordings are immediately flushed.
 * Use endRecording(handle) only after any asynchronous work has completed.
 * Names should contain only letters, digits, '_' or '-'.
 */
FLIPMANSDK_EXPORT RecordingId
beginRecording(const std::string& name, const std::string& filename = {});
/**
 * @brief Ends a named recording after its asynchronous work is idle.
 */
FLIPMANSDK_EXPORT void
endRecording(RecordingId id) noexcept;
/**
 * @brief Returns whether a recording handle is active.
 */
FLIPMANSDK_EXPORT bool
recordingActive(RecordingId id) noexcept;
/**
 * @brief Writes a numeric event into every currently active recording.
 */
FLIPMANSDK_EXPORT void
counter(const char* name, uint64_t value) noexcept;
/**
 * @brief Records structured diagnostic text only while a named recording is active.
 */
FLIPMANSDK_EXPORT void
event(const char* category, const char* name, const std::string& details);

/**
 * @brief Reset aggregate statistics only when no measured scopes are running.
 */
FLIPMANSDK_EXPORT void
reset();
/**
 * @brief Aggregate report; preferably call when instrumented work is idle.
 */
FLIPMANSDK_EXPORT std::string
summary();
/**
 * @brief Writes an aggregate report, replacing the destination file.
 */
FLIPMANSDK_EXPORT bool
writeSummary(const std::string& filename);

/**
 * @brief RAII timer for a registered source location; supports nested scopes.
 */
class FLIPMANSDK_EXPORT Scope final {
public:
    /**
     * @brief Starts a measured scope for a static source location.
     */
    explicit Scope(Site& site) noexcept;
    /**
     * @brief Records completed timing and restores the parent scope.
     */
    ~Scope() noexcept;
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    Site* site_ = nullptr;
    uint64_t startNs_ = 0;
    uint64_t childNs_ = 0;
    Scope* parent_ = nullptr;
    Mode mode_ = Mode::Off;
    unsigned depth_ = 0;
};

}  // namespace flipman::sdk::profile

#define FM_PROFILE_JOIN2(a, b) a##b
#define FM_PROFILE_JOIN(a, b) FM_PROFILE_JOIN2(a, b)
#define FM_PROFILE_SCOPE(category, name) FM_PROFILE_SCOPE_IMPL(category, name, __COUNTER__)
#define FM_PROFILE_SCOPE_IMPL(category, name, id)                                                   \
    static ::flipman::sdk::profile::Site FM_PROFILE_JOIN(_fm_profile_site_, id) { category, name }; \
    ::flipman::sdk::profile::Scope FM_PROFILE_JOIN(_fm_profile_scope_, id) { FM_PROFILE_JOIN(_fm_profile_site_, id) }
#define FLIPMAN_PROFILE_SCOPE(category, name) FM_PROFILE_SCOPE(category, name)
