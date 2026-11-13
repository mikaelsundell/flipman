# Building and packaging Flipman

`build_app.sh` and `build_app.bat` configure and build the SDK and developer
applications with configuration-specific dependency prefixes.

## Development conventions

See [the development guide](../AGENTS.md) for SDK ownership, header documentation,
Python bindings, UI, diagnostics, verification and PR conventions. `.clang-format`
defines the shared C++17 style: four-space indentation and a 120-column limit.
Format only the files touched by a change:

```sh
./scripts/clang-format.sh sources/flipmansdk/render/displaycolorspace.cpp
```

`scripts/clang-format.bat` provides the same file/directory entry point on Windows.
Both wrappers use Python 3 and `scripts/format_sources.py`. With no arguments,
they format first-party C++ headers, sources and Objective-C++ in `sources` and
`CTest`; vendored code is excluded. Pass `--check` to report formatting changes
without writing files. The driver requires clang-format 21 or newer, prefers
Xcode's formatter on macOS, and accepts `CLANG_FORMAT` for an explicit executable.

The shared configuration puts quoted local/private headers first, then a blank
line and one angle-bracket block ordered by SDK, external, Qt and C++ standard
headers, without gaps between those categories. The driver also keeps the file banner, pragma,
namespace bodies and guard clauses visually separated. It compacts `Data` structs
with no gaps between fields or before `Data d;`. Group fields by type and keep
the owner pointer last; review initialization and destruction dependencies
when changing their order. Private SDK headers live
in each component's `private/` directory; for example, renderer implementation
files include `"private/ociogpu.h"`. These headers are absent from the installed
public API.

## macOS

Supply a matching dependency prefix explicitly. From any working directory:

```sh
export THIRDPARTY_DEBUG_DIR=/path/to/dependencies/arm64.debug
export THIRDPARTY_RELEASE_DIR=/path/to/dependencies/arm64.release
export DeckLinkSDK_ROOT="/path/to/Blackmagic DeckLink SDK"
./build_app.sh debug --configure-only
./build_app.sh release --configure-only
# Open build.debug/flipman.xcodeproj or build.release/flipman.xcodeproj.
./build_app.sh debug --python
./build_app.sh release --python
# For RAW decoding, also export BLACKMAGIC_RAW_ROOT and add --braw.
```

`THIRDPARTY_DIR` is also accepted for a single matching configuration. For `all`,
provide both configuration-specific variables. `--sdk-only` disables interactive
demos and removes the DeckLink SDK requirement. SDK tests are controlled by
CMake's `BUILD_TESTING` option (on by default).

After configuration, `./test_app.sh debug|release|all` refreshes the cached
project, builds the aggregate test target and runs CTest with matching runtime
dependencies. `test_app.bat` provides the same entry point for Windows once the
SDK platform port is available. Both use the shared Python 3 controller and
produce JSON/JUnit reports under `build.test-results`. See
[test commands and reports](../CTest/README.md#run-all-tests) for filtering,
`--no-build`, required-fixture checks and output options.

The script generates `build.debug` and `build.release` without deleting them.
After an Xcode/dependency upgrade, add `--fresh` to discard stale CMake cache
entries while retaining the build directory.
It uses `CMAKE_OSX_SYSROOT=macosx`, so the active Xcode SDK is selected instead
of a stale SDK version path. `--target=VERSION` optionally sets the minimum OS,
not the SDK version. The default minimum is the running macOS
major version (for example 26.0 on macOS 26.7). Direct CMake configuration uses
the same default; an explicit CMake or environment deployment target takes
precedence. Debug exports `DYLD_IMAGE_SUFFIX=_debug`; Release clears
it. These same runtime settings are needed when launching from Xcode.

The main Flipman app target is still disabled and needs a separate application
migration. Current default builds compile the SDK, tests and interactive demos.
Do not mistake a generated Xcode project for a distributable Flipman app.

## Packaging an app bundle

Once an app bundle is available, packaging can run independently of compilation:

```sh
./scripts/package_app.sh --bundle /path/to/Flipman.app \
    --prefix "$THIRDPARTY_RELEASE_DIR" --config Release \
    --output deploy.release --dmg
```

Or append `--deploy --bundle=/path/to/Flipman.app --dmg` to a single-configuration
`build_app.sh` invocation (`--github` is an alias for `--deploy`; nothing is
uploaded). Packaging copies the app into `deploy.CONFIG`, runs `macdeployqt`,
sanitizes Mach-O references/rpaths, checks for external absolute references,
restores signatures, and produces a version/date/architecture/configuration ZIP.
`--dmg` adds a compressed DMG with an Applications link and the Flipman GitHub
link. Existing bundle/archive outputs are refused instead of overwritten.

Signing defaults to ad-hoc for local verification. Set `DEVELOPERID_IDENTITY`
and pass `--sign` for Developer ID signing. Notarization and publication are
separate future steps. The original build bundle is not modified by packaging.

See [Blackmagic runtime handling](blackmagic-runtime.md): DeckLink requires an
installed Desktop Video driver; an optional RAW framework can be bundled.
Python SDK bindings currently remain a standalone extension. Embedding Python
and its runtime in the application is a separate integration.

## Windows

`build_app.bat` provides the configure/build entry point, argument names and
configuration-specific dependency prefixes. It warns about the
pending platform port before invoking CMake. Flipman's SDK currently depends directly
on macOS frameworks and Objective-C++; a Windows build/deployment would be
misleading until those dependencies are ported.
`scripts/deploywin.py` is retained for that future integration, requires
`pefile`, and is not currently invoked or validated on Windows.

## Viewer demo

`testviewer` accepts an optional media path. In Xcode, add it under Edit Scheme → Run →
Arguments → Arguments Passed On Launch. Keep `DYLD_IMAGE_SUFFIX=_debug` in the
Debug scheme's environment. For a development BRAW clip, also set
`FLIPMAN_BRAW_RUNTIME_DIR` to the directory containing `BlackmagicRawAPI.framework`.

With no path, the demo opens an empty viewer. Drop a single local movie or image
onto the viewer to load or replace media and reset the timeline. Failed drops
report an error in the status bar and preserve the current media. Media opening
is performed in both Debug and Release builds, independently of assertions.
