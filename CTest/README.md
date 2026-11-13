# SDK regression tests

The former `sources/tests/testsdk` runner now lives here. Existing checks remain
in `sdk.cpp`, with one named CTest invocation per group instead of hard-coded
booleans in main. `flipmansdk_tests --list` lists the native cases; unknown names
return status 2. Every invocation uses a unique `QTemporaryDir` for output and
removes it afterward. Files/sequences and shader/FX fixtures are generated for
ordinary correctness tests; external media is never modified.

## Run all tests

After configuring a checkout with `build_app.sh` or `build_app.bat`, run:

```sh
./test_app.sh debug
./test_app.sh release
./test_app.sh all
```

On Windows, use `test_app.bat Debug`, `test_app.bat Release`, or
`test_app.bat All`. Both wrappers use the same Python 3 controller,
`CTest/run_all.py`; Debug is the default when no configuration is given.
The controller refreshes the existing CMake project with `BUILD_TESTING=ON`,
incrementally builds `flipman_tests`, then runs every registered CTest serially.
Build jobs default to at most six; `--jobs N` changes build parallelism only.
It uses the build cache's dependency prefix and feature/fixture settings, so
Python and BRAW tests run when those features were configured. Their enabled
state is printed and recorded; the controller does not silently enable optional
SDKs or download missing media.

The aggregate target includes the SDK tests, standalone header checks, enabled
Python extension and viewer-drop regression when demos are enabled. All Python
behavior tests already run through CTest. No application executable, embedded
interpreter, MCP server or DeckLink hardware is needed by the controller.
Native GPU and codec cases still require working host graphics/codec access.
Manual performance benchmarks remain separate from the correctness suite.

On macOS the controller sets `DYLD_IMAGE_SUFFIX=_debug` for Debug and clears it
for Release, including configure/build steps. Windows test processes prepend
the configuration-matched directories in CMake's generated
`test-runtime-CONFIG.txt` to `PATH`. The batch wrapper and runtime wiring are
provided for the pending Windows SDK port; Windows execution is not yet
validated, and the SDK still depends on macOS frameworks/Objective-C++.

Useful options, also accepted by the batch wrapper:

```sh
./test_app.sh all --list
./test_app.sh debug --no-build
./test_app.sh release --label cpu
./test_app.sh debug --test '^flipman[.]sdk[.]image_'
./test_app.sh all --exclude-label data
./test_app.sh all --fail-on-skip
./test_app.sh debug --output /absolute/path/new-results
./test_app.sh debug --build-dir /absolute/path/custom-build --cmake /path/to/cmake
```

`--list` lists registered tests and labels without building or running them.
`--label`, `--exclude-label` and `--test` pass regex filters to CTest. Empty
selections and selections containing only disabled tests fail a run.
`--build-dir` requires a single configuration. Use `--no-build` only with current
binaries and an enabled test configuration. Configure/build failures stop that
configuration before stale binaries can run; `all` still attempts the other
configuration and returns failure if either fails.

Results default to the ignored `build.test-results/<run>/` directory. The root
`summary.json` records overall status and configuration results; each
configuration has configure/build logs (unless `--no-build`), `tests.json`,
`ctest.log` and JUnit `ctest.xml`. Per-case status distinguishes passes,
failures, enabled skips, disabled tests and tests that did not run. Existing
output directories are refused rather than overwritten. Enabled skips are
reported as `passed_with_skips` when no tests fail; `--fail-on-skip` instead
returns failure. The intentionally disabled timer case does not count as an
enabled skip. For media validation, also consider `FLIPMAN_REQUIRE_TEST_DATA`
below. Generated reports are local build outputs, separate from curated
benchmark reports in `docs/benchmarks`.

`flipman.test_app_runner` checks the controller's configuration/runtime handling,
path quoting, build-failure propagation, test filters, skip reporting and
subprocess exit status using disposable fixtures. It requires Python 3.9+ and
does not build or launch the application recursively.

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
each public header independently, including the generated export header.

## Suites and labels

| Label | Coverage |
| --- | --- |
| `sdk;native;cpu` | File/sequence grouping, FPS, rational time/ranges, SMPTE, image classification/layout/type conversion, timeline configuration, media lifecycle, shader parsing/compilation and FX registry, shader contracts |
| `sdk;native;integration;data` | Clip metadata, media processing, reader/writer plugins, NV12 media, large-image conversion and concurrent sequence reading |
| `gpu` | Native offscreen rendering and UYVY round trips, plus synthetic Python offscreen rendering |
| `python;sdk` | Time/range validation, Unicode files/metadata, image bytes/layout/lifetimes, shader data, image write/read, runtime ownership/thread checks, rendering/profiling |
| `profile` | Nested/cross-thread recordings, counters/events, aggregate/reset/slow ring, logging categories, unset/empty/enabled/cached JSONL destinations |
| `display` | SDR display mappings, ICC import/export, profile value semantics and surface-format preservation |
| `writer` | Generated MOV/MP4 encoding/decoding, ProRes/H.264/HEVC, channel order, fractional FPS, title metadata, native NV12 storage, cursor/range and failure handling |
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

`flipman.sdk.image_converter` checks native Metal conversion against CPU
references for RGB/RGBA/BGRA UInt8/Half/Float, odd widths, alpha, HDR values,
GPU texture input and CoreVideo IOSurface output.
`flipman.sdk.quicktime_writer` exercises generated video round trips and writer
lifecycle errors. Neither uses performance thresholds.

Build `flipman_imageconverter_benchmark` explicitly in Release for completed
CPU/NEON versus Metal timings. See [conversion benchmarks](../docs/image-conversion.md).

`flipman.sdk.cpu_image_converter` verifies CPU-only conversion through the facade
and existing ImageBuffer wrappers, including all scalar formats, geometry/tags,
odd widths and identity ownership. The renderer channel test also compares
fused packed sampling with expanded RGBA textures and video-format transitions.
