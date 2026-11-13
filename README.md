Flipman
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](LICENSE)

Introduction
------------

Flipman is an experimental macOS movie and image viewer built around a C++ SDK,
Qt and Metal. It combines media decoding, GPU effects and color management with
frame-based navigation and SMPTE timecode.


Application
------------

`testviewer` is the current interactive front end for the SDK. It provides a
Metal-backed viewport for inspecting movies and images, with controls for image
scale, project resolution and color processing.

### Functionality

- Open movies and images from the command line or by dragging them into the viewer.
- Fit or zoom the image, scrub the timeline, and inspect the current frame,
  total frame count and embedded source SMPTE timecode.
- Select input, working and output gamut/gamma pairs, ACES viewing transforms
  and an independent display color-space tag for macOS ColorSync.
- Change project resolution and choose a background color from the viewer's
  context menu.

Build and deployment
--------------------

[Build and packaging scripts](docs/build-deploy.md) generate
`build.debug` and `build.release`, with ZIP/DMG packaging under `deploy.CONFIG`.
See [Blackmagic runtime handling](docs/blackmagic-runtime.md) for the DeckLink
driver requirement and optional Blackmagic RAW framework staging. The main
application target still needs migration before a Flipman app can be packaged.

SDK development
---------------

### Dependencies

- macOS, CMake 3.27 or newer, and a C++17 compiler.
- Qt 6 with Widgets and the Gui/ShaderTools private development packages.
- OpenImageIO for image I/O.
- OpenColorIO 2.5 or newer for GPU color management.

Use a compatible third-party prefix with matching Qt tools and libraries.
See [AGENTS.md](AGENTS.md) for C++ and header documentation conventions.

The SDK uses OpenColorIO for GPU input/output transforms, with ACEScg as the
default working space. `testviewer` enables management with input, working,
output/view and independent Display-tag controls. The embedded ACES Studio
config is Studio v4.0.0 / ACES 2.0 / OCIO 2.5.
See [color pipeline](docs/color-pipeline.md) and [Python API](docs/python-sdk.md).
The [ACES config license](sources/flipmansdk/resources/configs/LICENSE) is
retained with the resource and included in app packaging.

The registered [QuickTime writer](docs/quicktime-writing.md) exports MOV/MP4
video with ProRes, H.264 or HEVC through AVFoundation. RGBA8 packing uses Metal
when required; native layouts are handed off without CPU pixel conversion.
See [GPU image conversion and benchmarks](docs/image-conversion.md) for the
CPU/GPU conversion facade, shared QRhi backend and measured CPU/NEON
comparisons, including native packed sampling in the playback layer shader.

### Building and testing

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
SDK profiler and opt-in diagnostics. Optional
[Blackmagic RAW decoding](docs/blackmagic-runtime.md#braw-reader) is enabled with
`FLIPMAN_BUILD_BRAW=ON` and a supplied RAW SDK; reader options expose Kelvin, tint
and exposure independently of preview rendering.

After configuring, run `./test_app.sh debug`, `./test_app.sh release`, or
`./test_app.sh all` to build and run the registered native, Python and GPU tests.
`test_app.bat` uses the same controller for the future Windows SDK port. See
[test commands and reports](CTest/README.md#run-all-tests) for options and
JSON/JUnit output.

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

Project
-------

* GitHub page   
https://github.com/mikaelsundell/flipman

* Issues   
https://github.com/mikaelsundell/flipman/issues
