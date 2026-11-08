# Blackmagic runtime dependencies

## DeckLink

Set `DeckLinkSDK_ROOT` to the SDK root when building the DeckLink demo. Headers
and `DeckLinkAPIDispatch.cpp` are build inputs. The dispatch code is linked
statically; the SDK source tree does not need to ship inside the app.

The currently supported macOS SDK dispatch explicitly loads
`/Library/Frameworks/DeckLinkAPI.framework`. That framework and the hardware
services/drivers come from Blackmagic Desktop Video. Users of DeckLink hardware
must install a compatible Desktop Video release from Blackmagic Design:
https://www.blackmagicdesign.com/support/family/capture-and-playback

The packaging scripts do not copy the installed driver framework into the app.
An app-local copy would not be loaded by the current dispatch and would not
replace the system driver/services. The SDK dispatch license notice is included
in the bundle when `DeckLinkSDK_ROOT` is provided during packaging.

## Blackmagic RAW

RAW is a separate SDK/runtime. For a RAW-enabled app, set `BLACKMAGIC_RAW_ROOT`
to the SDK root containing
`Mac/Libraries/BlackmagicRawAPI.framework` (or directly to the directory
containing that framework). Set `BLACKMAGIC_RAW_LICENSE` to its accompanying
runtime redistribution license/notice. Packaging copies the complete framework
and the notice before deployment checks and signing. This option stages the
runtime; decoding requires the SDK to be built with `FLIPMAN_BUILD_BRAW=ON`.

No Blackmagic proprietary SDK source or installed drivers are vendored into
Flipman's Git tree by these scripts. Supply the SDK/runtime appropriate to the
application being built.

## BRAW reader

The SDK now includes `plugins::BrawReader`, adapted from
[brawtool.cpp](https://github.com/mikaelsundell/brawtool/blob/master/brawtool.cpp).
Enable it with `FLIPMAN_BUILD_BRAW=ON` and `BlackmagicRaw_ROOT` set to the SDK
root (or set `BLACKMAGIC_RAW_ROOT`). `build_app.sh --braw` passes these settings.
Without the option, the SDK still builds and the registry omits `.braw` support.
SDK headers remain private; public reader headers have no vendor dependency.

The SDK dispatch is compiled in, without linking an absolute SDK framework path.
At runtime it searches the app's `Contents/Frameworks` and executable-relative
framework locations first. For development, set `FLIPMAN_BRAW_RUNTIME_DIR` to
the directory containing `BlackmagicRawAPI.framework`; alternatively set
`BLACKMAGIC_RAW_ROOT` to the SDK root or its `Mac` directory. The vendor dispatch
caches the first loaded runtime for the process lifetime. Loading is deferred
until opening a BRAW clip; other formats do not require this runtime.

The reader uses the CPU pipeline and returns full-resolution, interleaved RGB
float32 frames in owned buffers. Clip processing defaults are preserved, with
the post-decode 3D LUT explicitly disabled. The reader adds no preview resize,
display LUT, overlay or export processing. Audio is not currently exposed.
The image's color-space/transfer enums remain Unknown; `Video` metadata exposes
`decodeGamma` and `decodeGamut` for the rendering pipeline. Other scalar and
numeric-array clip/frame metadata is exposed under `Camera`, and per-frame
`timecode` under `Timecode`. No sidecar or source media is written.

### Reader options

Use the existing `MediaReader::Options::values` map, also available as
`BrawReader::Options`. Omitting a key preserves the recorded processing value.

| Key | Value | Behavior |
| --- | --- | --- |
| `kelvin` | Integer, 2000–50000 | White-balance Kelvin override |
| `tint` | Integer, -150–150 | Signed white-balance tint override |
| `exposure` | Finite number, -20–20 | SDK exposure override |

These are input validation bounds; a clip/SDK can reject a narrower range during
decoding. Unknown keys, booleans, strings and nonfinite values are rejected.
Overrides are applied to cloned processing attributes on each decoded frame.

```cpp
plugins::BrawReader::Options options;
options.values.insert("kelvin", 5600);
options.values.insert("tint", -10);
options.values.insert("exposure", 1.0);

plugins::BrawReader reader;
if (reader.open(core::File(path), options)) {
    const av::Time decodedTime = reader.read();
    if (decodedTime.isValid()) {
        const core::ImageBuffer image = reader.image();
    }
}
// The same options can be passed to av::Media::open(file, options).
```

Each instance serializes its operations and owns its codec/callbacks. Independent
instances may read the same clip. `read()` returns the decoded frame timestamp;
`time()` points at the next frame. A valid `seek(range)` bounds subsequent reads
while `timeRange()` continues to describe the complete clip. EOF returns invalid
time without an error and clears the image. Failed reads also clear the image;
inspect `error()` to distinguish them. A successful seek/reopen clears an earlier
error. Jobs are flushed before resources are released.

### Verification

`flipman.sdk.braw_state` runs even when the SDK feature is disabled.
For actual decoding, set `FLIPMAN_BRAW_TEST_FILE` to a local BRAW clip with at
least three frames and `FLIPMAN_BRAW_TEST_RUNTIME_DIR` to its framework parent
(the test default is the configured SDK's `Mac/Libraries`).
`flipman.sdk.braw_decode` verifies pixel ownership, seeking, EOF, overrides and
reopening. `flipman.python.braw` verifies the public Python path when bindings
are enabled. Missing external fixtures skip these integration cases unless
`FLIPMAN_REQUIRE_TEST_DATA=ON`. These local paths stay in the build cache.
