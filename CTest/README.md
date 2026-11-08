# SDK regression tests

The former `sources/tests/testsdk` runner now lives here. Existing checks remain
in `sdk.cpp`, with one named CTest invocation per group instead of hard-coded
booleans in main. `flipmansdk_tests --list` lists the native cases; unknown names
return status 2. Every invocation uses a unique `QTemporaryDir` for output and
removes it afterward. Files/sequences and shader/FX fixtures are generated for
ordinary correctness tests; external media is never modified.

## Build and run

```sh
DYLD_IMAGE_SUFFIX=_debug cmake -S . -B build.sdk.debug -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$THIRDPARTY_DIR" \
    -DFLIPMAN_BUILD_DEMOS=OFF -DFLIPMAN_BUILD_PYTHON=ON -DBUILD_TESTING=ON
DYLD_IMAGE_SUFFIX=_debug cmake --build build.sdk.debug --target flipman_tests --parallel
DYLD_IMAGE_SUFFIX=_debug ctest --test-dir build.sdk.debug -L cpu --output-on-failure
DYLD_IMAGE_SUFFIX=_debug ctest --test-dir build.sdk.debug -L python --output-on-failure
DYLD_IMAGE_SUFFIX=_debug ctest --test-dir build.sdk.debug -L integration --output-on-failure
```

For Release use matching dependencies and omit the Debug image suffix. CTest
sets the matching runtime suffix before each process starts. Python cases use
the selected interpreter directly; use the generated `flipman-python` launcher
for manual runs. Do not use the protected system Python launcher with Debug Qt.
See [Python API](../docs/python-sdk.md) for lifetime and environment requirements.

`BUILD_TESTING=OFF` removes test targets. Python tests are registered only with
`FLIPMAN_BUILD_PYTHON=ON`. Demos and DeckLink hardware remain outside CTest.
`flipman_tests` builds the native runner, header checks, profiler tests and, when
enabled, the Python extension. It does not run them. Native header checks compile
66 headers independently, including the generated export header.

## Suites and labels

| Label | Coverage |
| --- | --- |
| `sdk;native;cpu` | File/sequence grouping, FPS, rational time/ranges, SMPTE, image classification/layout/type conversion, timeline configuration, media lifecycle, shader parsing/compilation and FX registry, shader contracts |
| `sdk;native;integration;data` | Clip metadata, media processing, reader/writer plugins, NV12 media, large-image conversion and concurrent sequence reading |
| `gpu` | Native offscreen rendering and UYVY round trips, plus synthetic Python offscreen rendering |
| `python;sdk` | Time/range validation, Unicode files/metadata, image bytes/layout/lifetimes, shader data, image write/read, runtime ownership/thread checks, rendering/profiling |
| `profile` | Nested/cross-thread recordings, counters/events, aggregate/reset/slow ring, logging categories, unset/empty/enabled/cached JSONL destinations |
| `timing` | Legacy randomized wall-clock timer benchmark; disabled by default |

CPU checks take no dependency on AVFoundation fixtures or GPU devices. CPU SDK
cases still create the SDK QApplication with Qt's offscreen platform; they are
not a claim that the SDK is Qt-free. Python value-type tests import without
creating a Qt runtime. GPU/AVFoundation cases need host graphics/codec access
and may fail under a restrictive sandbox; they must not silently pass on a
missing GPU. Individual cases have 30/60/120-second CTest timeouts.

The existing timeline test covers configuration and track management only;
it does not certify unfinished playback. No hardware DeckLink test is registered.

## External fixtures and benchmarks

`FLIPMAN_TEST_DATA_DIR` defaults to the repository's ignored `data/` directory.
Set it explicitly to use another fixture tree. Existing `data/build_data.sh` can
fetch that tree; CMake and CTest do not download assets. Missing required files
return 77 and are reported as **Skipped**, not passed. Set
`FLIPMAN_REQUIRE_TEST_DATA=ON` for validation runs that must fail on missing data.
Do not interpret a green run with skipped media cases as integration coverage.
The configured fixtures are external and not checksum-pinned by this migration.

The legacy timer test depends on randomized delays and a wall-clock tolerance;
it is preserved as an opt-in benchmark rather than a normal correctness gate.
Enable it with `FLIPMAN_ENABLE_TIMING_TESTS=ON`. Large-image tests are also
integration work, not small unit tests. Default plugin export checks write four
frames; concurrent image checks sample up to four frames per sequence, including
the first and last. `FLIPMAN_FULL_MEDIA_TESTS=ON` restores the legacy 100-frame
export and full-sequence workload. Increase CTest timeouts for those stress runs
when necessary. Use label selection during development.

The migration retains legacy coverage while adding native lifecycle and contract
regressions. It also corrects false-success paths where write failures failed to
update a test's result. New behavior changes should add focused cases rather
than extending a monolithic default run.

`flipman.sdk.shader_parser` covers GLSL token/AST function reflection, multiline
qualified signatures, prototypes, overloads, injected definitions, arrays, and
invalid declarations/delimiters. Full shader compilation remains covered by the
shader and GPU rendering cases. See [parser vendor notes](../3rdparty/README.md).

Optional BRAW tests cover disabled/error state, real-clip decode and Python reader
options; see [BRAW reader verification](../docs/blackmagic-runtime.md#verification)
for external fixture/runtime configuration.

`flipman.sdk.render_channels` verifies BGR/BGRA sampling against RGB/RGBA for
UInt8, Half and Float textures, alpha, and cached layout transitions.
`flipman.sdk.quicktime_channels` additionally compares a decoded BGRA movie frame
against an explicitly reordered RGBA reference. It uses the external fixture
`quicktime/iphone17pro film.mov` under `FLIPMAN_TEST_DATA_DIR`.
