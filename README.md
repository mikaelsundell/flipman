Flipman
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](LICENSE)

Introduction
------------

This project is a purely experimental exploration of Qt's accelerated 3D API and its shader abstraction layer, applied to QuickTime video playback and macOS Mach timing APIs. It serves as a sandbox for testing concepts, including real-time rendering, integration of QuickTime with Qt frameworks, and precise time synchronization using macOS Mach timers and SMPTE timecode generation.


Application
------------

todo: Insert application image here

### Functionality

todo: Insert application functionality here

Build and deployment
--------------------

Stageviz-derived [build and packaging scripts](docs/build-deploy.md) generate
`build.debug` and `build.release`, with ZIP/DMG packaging under `deploy.CONFIG`.
See [Blackmagic runtime handling](docs/blackmagic-runtime.md) for the DeckLink
driver requirement and optional Blackmagic RAW framework staging. The main
application target still needs migration before a Flipman app can be packaged.

SDK development
---------------

The SDK currently requires macOS, CMake 3.27 or newer, a C++17 compiler,
Qt 6 with Widgets and the Gui/ShaderTools private development packages, and
OpenImageIO. Use a compatible third-party prefix with matching Qt tools and
libraries. See [AGENTS.md](AGENTS.md) for C++ and header documentation conventions.

To configure and build the SDK independently of interactive demos and DeckLink,
set `THIRDPARTY_DIR` to your dependency prefix and run from the repository root:

```sh
DYLD_IMAGE_SUFFIX=_debug cmake -S . -B build.sdk.debug -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$THIRDPARTY_DIR" \
    -DFLIPMAN_BUILD_DEMOS=OFF
DYLD_IMAGE_SUFFIX=_debug cmake --build build.sdk.debug --target flipmansdk --parallel
DYLD_IMAGE_SUFFIX=_debug cmake --build build.sdk.debug --target flipmansdk_headers --parallel
```

The `_debug` suffix is for the macOS Debug dependency environment. For a Release
build, use a matching Release prefix and omit that suffix.

`flipmansdk_headers` compiles every public header in a separate translation unit
with the SDK's public consumer settings. The export header is generated inside
the build directory. This checks header independence; it does not run behavior
tests. The SDK tests now live under [CTest](CTest/README.md); configure with
`BUILD_TESTING=ON` and build `flipman_tests` to compile the regression targets.
The optional [Python SDK bindings](docs/python-sdk.md) are enabled with
`FLIPMAN_BUILD_PYTHON=ON`. See [profiling and logging](docs/profiling.md) for the
Stageviz-derived profiler and opt-in diagnostics. Optional
[Blackmagic RAW decoding](docs/blackmagic-runtime.md#braw-reader) is enabled with
`FLIPMAN_BUILD_BRAW=ON` and a supplied RAW SDK; reader options expose Kelvin, tint
and exposure independently of preview rendering.

`FLIPMAN_BUILD_DEMOS` defaults to `ON` to preserve the existing developer build.
That build includes interactive viewer, effects, styling, and DeckLink apps;
the DeckLink SDK is required when enabled. Automated tests are controlled
separately by `BUILD_TESTING`.

Third-party software
--------------------

[glsl-parser](https://github.com/graphitemaster/glsl-parser) by Dale Weiler is
vendored in [3rdparty/glsl-parser](3rdparty/glsl-parser) under its
[MIT license](3rdparty/glsl-parser/LICENSE). The SDK uses its lexer and AST for
effect function reflection, replacing line-based regex detection. Qt ShaderTools
continues to validate complete shaders. See [vendor notes](3rdparty/README.md)
for the pinned revision, local patches and supported GLSL syntax.

Flipman is licensed under [BSD-3-Clause](LICENSE); third-party components retain
their own licenses.

References
-------------

* AVFoundation   
https://developer.apple.com/documentation/avfoundation/

* Apple Technotes SMPTE   
https://developer.apple.com/library/archive/technotes/tn2310/_index.html

* Apple Mach Absolute Time Units   
https://developer.apple.com/library/archive/qa/qa1398/_index.html

* BlackmagicRAW SDK    
https://documents.blackmagicdesign.com/DeveloperManuals/BlackmagicRAW-SDK.pdf

* OpenImageIO Github    
https://github.com/AcademySoftwareFoundation/OpenImageIO

* QRhiWidget   
https://doc.qt.io/qt-6/qrhiwidget.html

* Qt GUI Private C++ Classes   
https://doc.qt.io/qt-6/qtguiprivate-module.html


Project
-------

* GitHub page   
https://github.com/mikaelsundell/flipman

* Issues   
https://github.com/mikaelsundell/flipman/issues

