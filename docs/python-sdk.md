# Flipman SDK Python API

The optional `flipman` extension calls the same shared SDK as native clients. It
uses nanobind, CPython 3.9 or newer (subject to the installed nanobind version),
and the selected Qt/OpenImageIO dependency environment. It does not embed an
interpreter or initialize Qt at import. This first binding layer intentionally
excludes unfinished timeline playback, audio, BRAW, multi-frame movie writing, and widgets.
There is no wheel/distribution packaging yet.

## Build and run

From the repository root, with `THIRDPARTY_DIR` set to a compatible Debug prefix:

```sh
DYLD_IMAGE_SUFFIX=_debug cmake -S . -B build.sdk.debug -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$THIRDPARTY_DIR" \
    -DFLIPMAN_BUILD_DEMOS=OFF -DFLIPMAN_BUILD_PYTHON=ON -DBUILD_TESTING=ON
DYLD_IMAGE_SUFFIX=_debug cmake --build build.sdk.debug --target flipman_tests --parallel
build.sdk.debug/python/Debug/flipman-python your_script.py
DYLD_IMAGE_SUFFIX=_debug ctest --test-dir build.sdk.debug -L python --output-on-failure
```

Set `Python_EXECUTABLE` and/or `nanobind_DIR` when discovery needs help. CMake
first searches the dependency prefix, then the selected interpreter's nanobind
package. It does not download dependencies. The generated `flipman-python`
launcher uses that interpreter, adds the module directory to `PYTHONPATH`, and
sets the matching Qt image suffix. It forwards all Python arguments.

On macOS, `DYLD_IMAGE_SUFFIX=_debug` must be present at process startup for the
Debug Qt environment. The
protected `/usr/bin/python3` launcher may strip DYLD environment settings; use
the generated launcher or the configured interpreter directly. Mixing Debug
Qt libraries with Release platform plugins can abort Python. `Runtime()` now
rejects a missing Debug suffix before creating QApplication. Do not mix this
SDK with a different Qt/PySide installation in the same process. For Release,
use matching Release dependencies; the generated launcher clears the suffix.

## Ownership and errors

Pure value/file/image/metadata operations do not need `Runtime`. `Runtime()`
creates a native `core::Application`, or borrows an existing SDK application.
Multiple runtimes share an owned application. An existing ordinary Qt/PySide
application is rejected: the SDK currently requires its own application services.
An externally owned application must outlive the bindings.

Create, use, and release runtimes and Qt-dependent objects on the main/application
thread. These methods check the thread and raise `RuntimeError` for invalid use.
`Media(runtime)` keeps the runtime alive. Borrowing an application does not
transfer ownership. Do not pass wrappers to worker threads for destruction.
`Runtime.processEvents()` services queued Qt work; there is no implicit GUI loop.

Python byte reads return owned copies. `ImageLayer.image()` and native image
results may share native data; `setPlaneBytes()` detaches before changing bytes.
Most other native value results retain the SDK's explicit sharing semantics.
Time/Fps/range bindings omit mutating setters to keep their initial API small.

Invalid Python arguments raise `ValueError`, missing frame/plane indices raise
`IndexError`, unsupported conversions raise `TypeError`, and operation/thread
failures raise `RuntimeError`. `Media.open()` is the exception: it returns a bool
and exposes the SDK's `Error`. Always check it before reading. A timeout returns
false and may leave an asynchronous open pending; close/reopen the media before
retrying. Calls currently retain the Python GIL, including bounded open waits.

## Timecode

Names and argument casing follow the SDK. Documented arguments support keywords.

- `Fps()`: invalid descriptor. `Fps(other)` copies it.
- `Fps(numerator, denominator=1, dropFrame=False)`: positive rational rate.
- `isValid()`, `numerator()`, `denominator()`, `dropFrame()`, `real()`, `seconds()`,
  `frameQuanta()`, `frameScale()`, `toString()`, `float(fps)`, equality/inequality.
  `toString()` preserves the native representation (frame duration).
- Static `guess(fps)`, `fps23_976()`, `fps24()`, `fps25()`, `fps29_97()`, `fps30()`,
  `fps47_952()`, `fps48()`, `fps50()`, `fps59_94()`, `fps60()`.
  Standard factories retain the current SDK's drop-frame flags.
- `Time()`, `Time(other)`, `Time(ticks, timeScale, fps)`; `isValid()`, `fps()`,
  `ticks()`, `timeScale()`, `frames()`, `seconds()`, `toString()`, `str(time)`,
  equality/inequality, addition/subtraction with matching scale and rate.
- Static `Time.zero(fps)`, `fromFrames(frame, fps)`, `fromSeconds(seconds, fps)`,
  `convert(time, fps)` and `convert(time, timeScale)`.
- `TimeRange()`, `TimeRange(other)`, `TimeRange(start, duration)`; `isValid()`,
  `start()`, `duration()`, `end()` (exclusive), `bound(time, loop=False)`,
  `intersects(other)`, equality/inequality. Duration must be positive.
  `bound` clamps/wraps to a full frame inside the range.
- `SmpteTime(time)`; `hours()`, `minutes()`, `seconds()`, `frames()`, `frame()`,
  `time()`, `isValid()`, `toString()`, `str(timecode)`.

```python
import flipman as sdk
fps = sdk.Fps.fps24()
start = sdk.Time.fromFrames(86400, fps)
print(sdk.SmpteTime(start))
span = sdk.TimeRange(start, sdk.Time.fromFrames(48, fps))
```

## Files, errors, metadata

- `Error()`, `Error(domain, message, code=0)`; `domain()`, `message()`, `code()`,
  `isValid()`, `hasError()`, `bool(error)`.
- `File()`, `File(path)`; `filePath()`, `absolutePath()` (parent directory),
  `fileName()`, `fileName(frame)`, `extension()`, `displayName()`, `dirName()`,
  `baseName()`, `size()`, `exists()`, `isValid()`, `fileRange()`, `str(file)`.
  Static `File.listDir(path)` returns sequence-grouped `File` objects.
- `FileRange()`; `size()`, `isValid()`, `hasFrame(frame)`, `frame(frame)`,
  `start()`, `end()` (inclusive file index), `insertFrame(frame, file)`.
- `MetaData()`, `MetaData(other)`; `isValid()`, `reset()`, `groups()`, `keys(group)`,
  `value(key)` / `value(group, key)`, `insert(key, value)` /
  `insert(group, key, value)`, `remove(key)` / `remove(group, key)`.
- `MetaData.Group`: Container, Video, Audio, Camera, Timecode, Production, Custom.

Metadata accepts `None`, bool, signed int64, float, Unicode strings, lists/tuples,
and string-keyed dictionaries, with at most 32 nested container levels. Returned
containers are copies. Missing values return `None`; native date-time values
return ISO strings. Other native QVariant types raise `TypeError`.

## Images

- `Size(width, height)` and `Rect(x, y, width, height)` expose their named component
  methods. Returned window/size values use these types rather than PySide wrappers.
- `ImageFormat(type)`; `type()`, `size()`, `isValid()`.
  `ImageFormat.Type`: Unknown, UInt8, Int8, UInt16, Int16, UInt32, Int32, UInt64,
  Int64, Half, Float, Double.
- `ImageBuffer()` (invalid), `ImageBuffer(width, height, type=ImageFormat.Type.UInt8,
  channels=4)` (descriptor only); call `allocate()` before accessing pixels.
- `isValid()`, `isAllocated()`, `dataWindow()`, `displayWindow()`, `imageFormat()`,
  `channels()`, `byteSize()`, `strideSize()`, `planeCount()`, `planeSize(plane)`,
  `planeStride(plane)`, `planeBytes(plane=0)`, `setPlaneBytes(data, plane=0)`,
  `detach()`, `requiresDecode()`, `isYCbCr()`, `isRgb()`.
- `packing()`/`setPacking(value)`, `subsampling()`/`setSubsampling(value)`,
  `pixelLayout()`/`setPixelLayout(value)`, `pixelRange()`/`setPixelRange(value)`:
  configure before allocating. Subsampled 4:2:0 requires even width/height;
  4:2:2 requires even width.
- `colorSpace()`/`setColorSpace(value)` and
  `transferFunction()`/`setTransferFunction(value)` tag source interpretation.
- Nested enums `Packing` (Interleaved, Planar, BiPlanar, Packed), `Subsampling`
  (None_, CS420, CS422, CS444), `PixelLayout` (Unknown, RGB, BGR, RGBA, BGRA,
  NV12, NV21, UYVY, YUYV, YVYU, VYUY, V210), `PixelRange` (Unknown, Full, Video).

`planeBytes` copies the complete native plane, including row padding;
`setPlaneBytes` requires `bytes` of exactly that size. It exposes no raw pointers
or borrowed memoryviews. Native layout/format support still applies.

## Media and image output

`Media(runtime)` exposes `open(file, timeoutMs=5000, *, options={})`,
`isSupported(extension)`, `close()`, `isOpen()`,
`isValid()`, `error()`, `file()`, `read()`, `seek(range)`, `time()`, `start()`,
`fps()`, `timeRange()`, `image()`, `metaData()`. Read returns the native `Time`;
check validity and errors at EOF. Video decoding uses AVFoundation; image input
uses OpenImageIO. This layer does not expose audio or Python Qt signals.

`writeImage(runtime, image, file)` writes one allocated interleaved RGB image
through the native writer registry and returns the actual output `File`.
It preserves SDK sequence naming: a `frame.####.png` pattern produces
`frame.0000.png`, and an unpatterned `frame.png` produces `frame.0.png`.
MOV/MP4 use the exact supplied path and produce a finalized single-frame video
(MOV defaults to ProRes 422, MP4 to H.264). Movie input requires an explicit
RGBA/BGRA layout. Errors from finalization raise `RuntimeError`. Multi-frame
encoding and codec options remain native SDK APIs; see [QuickTime writing](quicktime-writing.md).

```python
runtime = sdk.Runtime()
image = sdk.ImageBuffer(8, 4)
image.allocate()
image.setPlaneBytes(bytes([255, 0, 0, 255]) * 32)
output = sdk.writeImage(runtime, image, sdk.File("/tmp/frame.####.png"))
media = sdk.Media(runtime)
if not media.open(output):
    raise RuntimeError(media.error().message())
media.read()
pixels = media.image().planeBytes()
media.close()
```

## Rendering and shader data

`parseShader(source)` returns a `ShaderDefinition` with `isValid()`, `shaderCode()`,
`uniformBlock(binding=0, uniformName="Params")`, `error()`. This expands Flipman
directives and reflects function declarations using the
vendored GLSL parser. It reports lexer, delimiter and declaration errors as
`RuntimeError`; full body/type validation happens later in the shader compiler
(see [supported syntax](../3rdparty/README.md)). This parses source;
it does not compile or validate an effect entry contract. Effect entry functions
in the renderer take `(vec4 color, ivec2 pixel, ivec2 size)`; the contract also
accepts float pixel/size vectors. The old two-argument UV contract is obsolete.

`ImageEffect()` exposes `name()`/`setName(name)`, `group()`/`setGroup(group)`,
`isValid()`, `error()`, `shaderDefinition()`/`setShaderDefinition(definition)`.
`ImageLayer()` exposes `image()`/`setImage(image)`,
`imageEffect()`/`setImageEffect(effect)`, and `error()`.

`renderImage(runtime, image, width=0, height=0)` renders one image offscreen through
`RenderEngine`; zero uses the corresponding display-window dimension. It returns
an allocated interleaved RGBA half-float buffer. It requires a working GPU backend
(Metal on macOS), retains default native color behavior, and measures host render
time rather than hardware GPU time. Layer-stack rendering, parameter editing,
and explicit render transforms are not yet exposed by this convenience function.

Module enums `ColorSpace`, `TransferFunction`, and `WorkingSpace` expose the
current native enum names. These describe capabilities; an enum entry does not
promise that every conversion is implemented. See the native
`include/flipmansdk/render/render.h` for descriptions.

## Profiling

The `profile` submodule exposes `Mode.Off/Aggregate/Trace`, `setMode(mode)`, `mode()`,
`setSlowThresholdUs(microseconds)`, `setTraceCapacity(events)`,
`beginRecording(name, filename="")`, `endRecording(id)`, `recordingActive(id)`,
`counter(name, value)`, `event(category, name, details)`, `reset()`, `summary()`,
`writeSummary(filename)`. See [profiling and logging](profiling.md). State belongs
to the shared SDK, so native and Python clients see the same measurements.

## Later application integration

The application can import this extension from its embedded interpreter's module
path after creating `core::Application`; `Runtime` will borrow that application.
No second QApplication or application window is needed. Interpreter setup,
PySide integration, app/session wrappers, and a script editor are separate work.
The ABI-tagged extension and shared SDK must match the application's Python and
Qt runtimes; this is not a stable-ABI Python wheel.

## BRAW decode options

`Media.open(file, timeoutMs=5000, *, options={})` forwards the optional string-keyed
options dictionary to the native reader. Values use the metadata conversion
rules. `Media.isSupported(extension)` reports the registered readers for this
build. For BRAW, configure `FLIPMAN_BUILD_BRAW=ON` and provide the RAW runtime as
explained in [Blackmagic runtime handling](blackmagic-runtime.md).

```python
runtime = flipman.Runtime()
media = flipman.Media(runtime)
if media.open(flipman.File(path), options={"kelvin": 5600, "tint": -10, "exposure": 1.0}):
    time = media.read()
    if time.isValid():
        image = media.image()  # full-resolution RGB float32
```

BRAW accepts only the three RAW decode overrides above; see the native reader
option bounds in the linked guide. File cloning, exports, resize, LUTs and
metadata overlays are outside the reader. `open` returns False on native errors;
inspect `media.error()`. Unsupported Python value types raise TypeError.

## GPU color management

`ColorSettings()` has writable `enabled` (default `False`), `configFile` (empty
uses the embedded Studio v4 / ACES 2 / OCIO 2.5 config), `workingSpace` (default
`"ACEScg"`), `display` (default `"Rec.1886 Rec.709 - Display"`), and `view` (default
`"Un-tone-mapped"`). The working space must be a non-data scene-referred config space. Encoded
spaces such as `"ARRI LogC3 (EI800)"` are supported. Effects and the existing
layer blend operate in that selected encoding; linear-light blending for log
working spaces is deferred. `display` also accepts a composable color-space identifier;
`display="Raw"` or `view="Raw"` bypasses the final output transform.

- `combinedColorSpace(gamut, encoding, configFile="")` returns a color-space identifier
  for an independent pair, e.g. `("ARRI Wide Gamut 3", "Gamma 2.4")`. The embedded
  config composes its known curves and linear RGB gamuts; external configs resolve
  only their existing pairs. Unsupported pairs raise `RuntimeError`.
- `colorSpaces(configFile="")` returns source/working color-space names.
- `colorSpaceFamily(name, configFile="")` returns the config family path, e.g.
  `"Input/ARRI"`, used for grouped source menus.
- `workingSpaces(configFile="")` lists non-data scene-referred working encodings.
- `colorProfiles(configFile="")` returns dictionaries with `name` (exact OCIO
  identifier), `gamut`, `encoding` and `family` presentation metadata. The paired
  viewer controls use these supported combinations, including the embedded config’s
  composable gamut/curve pairs.
- `colorDisplays(configFile="")` returns output display names.
- `colorViews(display, configFile="")` returns view names for that display.
- `ImageLayer.inputColorSpace()` / `setInputColorSpace(name)` accesses the exact
  OCIO source override; empty means use supported image metadata. Layer copies
  share this setting, like the existing layer image/effect fields.
- `renderImage(runtime, image, width=0, height=0, *, colorSettings=ColorSettings(),
  inputColorSpace="")` renders using GPU transforms when enabled. Invalid settings,
  unsupported source tags, or shader/resource errors raise `RuntimeError`.

```python
settings = flipman.ColorSettings()
settings.enabled = True
settings.view = "ACES 2.0 - SDR 100 nits (Rec.709)"
preview = flipman.renderImage(runtime, image, colorSettings=settings,
                             inputColorSpace="ARRI LogC3 (EI800)")
```

Select the source that describes the decoded pixels, which may differ from the
camera original after a decoder's processing. Use `Un-tone-mapped` for already
rendered video when another tone-mapping treatment is unwanted. `renderImage`
returns an offscreen buffer and performs its existing readback; interactive
playback does not read pixels back. See [GPU color management](color-pipeline.md).
