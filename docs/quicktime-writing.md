# QuickTime video writing

`plugins::QuicktimeWriter` implements `MediaWriter` using AVFoundation. It is
compiled with the SDK's Objective-C++ sources and registered for `mov` and
`mp4` before generic image writers. Containers are finalized by `close()`;
check its result before using the output file. The destructor also finalizes an
open writer, but cannot report errors to the caller.

## Configuration and lifetime

```cpp
plugins::QuicktimeWriter writer;
writer.setFps(av::Fps(24000, 1001));
plugins::MediaWriter::Options options;
options.values.insert("codec", "prores422hq");
if (!writer.open(core::File("/tmp/output.mov"), options)) {
    // Inspect writer.error().
}
// For each prepared output image:
// if (!writer.write(image).isValid()) inspect writer.error().
// After the last frame:
// if (!writer.close()) inspect writer.error().
```

| Option | Behavior |
| --- | --- |
| `codec` | `h264`, `hevc`, `prores422proxy`, `prores422lt`, `prores422`, `prores422hq`, `prores4444`, `prores4444xq`. MOV defaults to ProRes 422; MP4 defaults to H.264 and accepts H.264/HEVC. |
| `bitrate` | Positive integer bits/second for H.264/HEVC; omitted means system default. |
| `timeoutMs` | Positive integer, at most 120000; defaults to 30000. Bounds encoder backpressure and completion waits. |

The encoder must be available on the host. Invalid options, unavailable codec
settings, unsupported frame layouts and encoder failures return an SDK error;
there is no silent codec fallback. Existing files are never overwritten.
Opening another output requires closing the current one first. Closing a movie
with no frames fails. A failed export can leave an incomplete file.

Set frame rate, metadata and an optional intended time range before the first
frame. Dimensions, pixel layout, gamut and curve remain fixed thereafter.
`seek()` configures the intended range before writing; it cannot rewrite frames
in an active movie. A configured range limits the number of appended frames.
`write()` returns the logical timestamp of the accepted frame, and `time()`
returns the next logical timestamp. Container presentation starts at zero and
uses the exact rational frame rate. A nonzero range start affects the logical
SDK cursor; it does not create an encoded SMPTE timecode track.

Methods serialize access, but construct, use and destroy the writer on one
owner thread. Frame submission and finalization are synchronous export
operations and may wait for the encoder or GPU. For worker exports, create the
writer on that worker; the viewer's realtime rendering path does not invoke it.

## Pixel and color handling

Native NV12/BGRA and floating-point RGBA storage is wrapped with CoreVideo release
callbacks that retain shallow image handles until the encoder releases them.
Where CoreVideo cannot wrap RGBA8, the shared QRhi `ImageConverter` uploads the
CPU source once and packs it on native Metal into a pooled BGRA IOSurface.
The output surface goes directly to AVFoundation after GPU completion. This
export-only step performs no CPU pixel conversion, color transform, OCIO
evaluation or GPU readback. Upload staging still copies CPU source bytes; this
is not a zero-copy path. Native wrapped layouts avoid this conversion. See
[GPU image conversion](image-conversion.md) for API and performance results. Keep
submitted storage immutable until `close()` returns. Releasing the caller’s
handle is safe; mutating another shared handle is not.

Supported inputs are NV12 UInt8 (bi-planar 4:2:0, even dimensions), BGRA UInt8,
and RGBA UInt8/Half/Float (interleaved, four channels). AVFoundation may convert
pixel formats internally for the selected encoder. These internal operations
are not a promise of hardware-only or end-to-end zero-copy encoding. Codec
choice controls bit depth and alpha preservation; H.264 discards alpha, while
ProRes 4444 is the alpha-capable export choice.

Output color properties and CoreVideo attachments agree with the submitted
image tags: Rec.601/SMPTE-C, Rec.709, Display P3 or Rec.2020 primaries, with sRGB,
linear or the SDK's Rec.709/Gamma 2.4 convention. Unknown tags assume Rec.709.
Other gamma/log/ACES encodings are rejected rather than mislabeled. Prepare the
intended output encoding through the GPU color pipeline before submitting it.
There is no automatic ACES view or display-profile transform during writing.

Container `title`, `author` and `description` metadata are supported. Audio is
explicitly unsupported because the current `AudioBuffer` API has no sample-data
accessors. Multi-frame movie export remains a native API; Python `writeImage`
can produce a single finalized MOV/MP4 at the exact requested path.

The synthetic `flipman.sdk.quicktime_writer` CTest encodes and decodes generated
fixtures to check codec selection, channel order, frame counts, fractional FPS,
metadata, native NV12 handoff, retained buffer storage and failure handling.

AVFoundation references: [sample buffer submission](https://developer.apple.com/documentation/avfoundation/avassetwriterinput/append(_:))
and [writer finalization](https://developer.apple.com/documentation/avfoundation/avassetwriter/finishwriting(completionhandler:)).
