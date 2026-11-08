# Building and packaging Flipman

The entry points follow Stageviz's `build_app.sh` / `build_app.bat` workflow,
with Flipman's dependency set. OpenUSD, USD plugins, PySide and Stageviz's Python
runtime deployment are not part of this workflow.

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

The script generates `build.debug` and `build.release` without deleting them.
After an Xcode/dependency upgrade, add `--fresh` to discard stale CMake cache
entries while retaining the build directory.
It uses `CMAKE_OSX_SYSROOT=macosx`, so the active Xcode SDK is selected instead
of a stale SDK version path. `--target=VERSION` optionally sets the minimum OS,
not the SDK version. Like Stageviz, the default minimum is the running macOS
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

`build_app.bat` adapts the Stageviz-style configure/build entry point, argument
names and configuration-specific dependency prefixes. It warns about the
pending platform port before invoking CMake. Flipman's SDK currently depends directly
on macOS frameworks and Objective-C++; a Windows build/deployment would be
misleading until those dependencies are ported. The Stageviz-derived
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
