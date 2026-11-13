# Profiling and diagnostic logging

The SDK provides `profile.h/.cpp` and the optional `PerfScope` phase logger.
Namespaces, macros and environment variables use Flipman names. State lives in
the shared SDK, so native clients and Python observe the same profiler. Profiling
is off by default. Regular warning/error reporting is independent of profiling.

## Aggregate timings and recordings

```cpp
#include <flipmansdk/core/profile.h>

flipman::sdk::profile::setMode(flipman::sdk::profile::Mode::Aggregate);
{
    FM_PROFILE_SCOPE("media", "my_operation");
    // measured work
}
const std::string report = flipman::sdk::profile::summary();
```

`FLIPMAN_PROFILE_SCOPE` is an alias for `FM_PROFILE_SCOPE`. Category and name must
be static string literals. Sites are registered for process lifetime; do not
create temporary `profile::Site` objects. Scopes support nested inclusive and
exclusive wall times, calls, averages and maxima across threads. Aggregate mode
keeps counters in memory; it does not print on every call. `Trace` additionally
retains a bounded ring of slow completed calls, controlled by
`setSlowThresholdUs()` and `setTraceCapacity()`.

Named recordings capture instrumented scopes and numeric/text events even when
normal profiling mode is Off:

```python
import flipman as sdk
recording = sdk.profile.beginRecording("decode", "/tmp/flipman-decode.trace")
if not recording:
    raise RuntimeError("could not start profiler recording")
try:
    # Open/read/render using the SDK. Wait for asynchronous work to finish.
    sdk.profile.counter("requested_frames", 24)
finally:
    sdk.profile.endRecording(recording)
print(sdk.profile.summary())
```

Names permit letters, digits, underscores and hyphens. A nonempty filename is
used as supplied, appending to an existing file. Parent directories must exist.
Without a filename, `FLIPMAN_PROFILE_REPORT=/tmp/flipman` selects
`/tmp/flipman.<recording-name>.trace`; otherwise output goes to stdout. Recording
start announces its destination to stderr. Files are flushed synchronously.
Multiple recordings may overlap and receive events from all measured threads.
The text format contains recording ID, steady-clock timestamp, thread, nesting
depth, BEGIN/END/COUNT/EVENT, scope name, and duration/counter value.

`event(category, name, details)` writes text only while recording; embedded
newlines are flattened. `recordingActive(id)` checks handles. Stop recordings
only after the asynchronous work of interest has finished. Call `reset()` only
when measured work is idle; it clears aggregate statistics and the slow-call
ring. `writeSummary(filename)` writes the current aggregate report.

## Optional JSONL phase timings

Set the destination before launching:

```sh
FLIPMAN_PERF_LOG=/tmp/flipman-phases.jsonl \
    build.sdk.debug/python/Debug/flipman-python your_script.py
```

Native code uses `core::PerfScope phase("category.operation")`. Unset or empty
`FLIPMAN_PERF_LOG` disables clocks, JSON serialization and file writes. The path
is cached at first use; restart to change it. Output appends JSON lines with
`phase`, `start_ms` (steady clock), `wall_start_ms` (Unix milliseconds),
`duration_ms`, and `thread`. Completion order may differ from start order.
Thread writes are serialized within a process. An unavailable destination
produces no output; there is no rotation or cross-process coordination.

Current instrumentation covers media open/read/seek/open-wait, QuickTime asset
loading/read/seek and buffer-copy scopes, image conversion, engine frame
submission, offscreen rendering, and the Python render convenience function.
A submitted/open-request scope is not proof of asynchronous completion.
These measurements include synchronous callees, waits and profiler overhead;
they are not GPU hardware timings or CPU utilization. Do not sum overlapping
or nested wall times. Verify final responsiveness with profiling disabled.

## Qt diagnostics

Flipman uses Qt logging categories for its routine media/render diagnostics:

```sh
QT_LOGGING_RULES="flipman.media.debug=true" \
    build.sdk.debug/python/Debug/flipman-python your_script.py

QT_LOGGING_RULES="flipman.render.debug=true" \
    build.sdk.debug/python/Debug/flipman-python your_script.py
```

Both categories default to warning severity, so normal operation is quiet.
Warnings/errors remain visible. `qCDebug()` avoids evaluating its stream
arguments when disabled. QuickTime's attachment inspection is also gated.
The base `RenderOutput` diagnostic PNG dump runs only when render debug logging
is enabled. Enable these diagnostics for bounded investigations rather than
performance measurements. The rules apply to Debug and Release builds; they
are independent of `FLIPMAN_PERF_LOG` and explicit profiler recordings.
