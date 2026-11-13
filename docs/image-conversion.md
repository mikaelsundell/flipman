# Image conversion

`render::ImageConverter` is the common facade for explicit CPU conversion and
GPU command recording. It selects no backend implicitly and does not move
pixels between CPU and GPU without an explicit recording/readback operation.

## Implementations

- `core/imageconversion.cpp` contains the shared portable scalar and NEON CPU
  kernels, with declarations in `core/private/imageconversion.h`.
  `ImageBuffer::convert()` and `ImageConverter::convertCpu()` delegate here.
  Core CPU storage has no dependency on rendering-device initialization.
- Private `render::RhiImageConverter` owns the GPU conversion implementation.
  The public facade borrows the caller's QRhi device; on macOS the native
  Metal device is used. Direct3D and other supported QRhi backends can use the
  same implementation, subject to backend validation.
- Private `MetalImageConverter` composes the public facade with a CoreVideo
  IOSurface adapter. It owns its export device/pool and returns a completed,
  retained BGRA surface to the QuickTime encoder. It does not inherit the CPU
  kernels. Native BGRA/NV12/floating RGBA wrapping remains available.
- `render/private/pixelsampling.h` shares packed-pixel GLSL between the GPU
  converter and the render engine. The layer shader now samples native
  interleaved 1/2/3-channel data directly, instead of expanding it on the CPU.
  Channel defaults, RGB/BGR normalization, color transforms and effects occur
  in the existing layer pass. Four-channel and native video texture paths
  remain direct. No additional playback pass or readback is introduced.

CPU conversion is available on every platform, including macOS. GPU recording
uses the caller's device, including Metal on macOS; it never silently falls
back to CPU. Windows execution has not been validated by these macOS results.

## CPU output

```cpp
using namespace flipman::sdk;
auto rgba = render::ImageConverter::convertCpu(source, 4);
auto half = render::ImageConverter::convertCpu(source, core::ImageFormat::Half, 4);
```

The channel-only overload keeps the existing NEON paths for UInt8/Half/Float
RGB-to-RGBA and portable scalar implementations. The format/channel overload
retains the existing integer/Half/Float/Double conversion semantics. There is
no RGB/BGR swizzle or color-space evaluation. Geometry and source tags carry
through. Missing components are filled with normalized 1, matching the legacy
API. Format/channel identity conversion detaches value state and provides
independent writable storage; channel-only identity conversion shares storage.
CPU input must be valid allocated RGB-like interleaved data with no chroma
subsampling, as required by `ImageBuffer::convert()`.

## GPU recording and ownership

```cpp
render::ImageConverter converter(rhi); // borrowed device
// Within an active frame, before submitting/completing it:
if (!converter.record(commands, sourceImage, destinationTarget)) {
    // Inspect converter.error().
}
```

Use the public header `flipmansdk/render/imageconverter.h`. The existing GPU
`convert()` overloads remain compatibility forwards to `record()`. The CPU
operation needs no GUI application or device. A facade constructed without a
device rejects GPU recording.

GPU conversion supports interleaved RGB/RGBA/BGRA UInt8/Half/Float ingress and
RGBA8/BGRA8/RGBA16F/RGBA32F texture input/output. Three-channel input receives
opaque alpha. Dimensions stay fixed. There is no gamma conversion, OCIO
operation, resizing or alpha premultiplication. Normalized integer targets
clamp; floating targets preserve negative/HDR values within representable range.

Pass an active command buffer and created single-color, single-sample texture
render target on the same device. Resources/pipelines are cached. Create,
record, release and destroy GPU resources on the QRhi thread. The device must
outlive the converter. Record one conversion per instance per frame; complete
pending GPU work before resizing resources or changing destination format, and
before resource release. Release cached resources before destroying a borrowed
source texture; keep source and destination alive until GPU completion.

CPU ingress stages/uploads source bytes once. Texture input is already
GPU-resident and needs no upload. Neither recording overload waits or reads
back. The private encoder adapter waits before handing its IOSurface to
AVFoundation. `ImageBuffer::convert()` remains CPU-returning, with no implicit
GPU transfer or synchronization.

## Reproducing the benchmark

Build with Release dependencies and omit `DYLD_IMAGE_SUFFIX`:

```sh
cmake --build build.release --config Release --target flipman_imageconverter_benchmark
QT_QPA_PLATFORM=offscreen build.release/CTest/Release/flipman_imageconverter_benchmark 30
```

The executable prints JSON statistics. It runs five warmups and 30 measured
iterations per path and resolution, reusing GPU resources after warmup. Shader
compilation and setup are excluded. CPU conversions include destination
allocation through the existing SDK API. GPU wall times include completed GPU
work (`endOffscreenFrame` and `finish`), not just dispatch. Upload/readback costs
are explicitly measured. `thread_cpu_ms` measures the calling thread only;
`record_ms` measures recording/upload staging before completion, where applicable.
The previous Core Image path locks/unlocks its output to include visibility and
completion; that lock overhead is included. CPU scalar and NEON RGBA/BGRA
swizzles are benchmark-only references, not existing ImageBuffer swizzle APIs.

## Recorded results

Measured 2026-11-09, Apple M1, macOS 26.7.1, Qt 6.10.1, Release,
30 iterations after five warmups. Values are milliseconds per frame.
These are local measurements, not timing gates or playback FPS predictions.
CPU rows return CPU pixels; GPU rows retain output on GPU unless explicitly
marked readback. The resident texture row starts with already-uploaded input.
Encoders are outside the conversion timings. Cold startup, concurrent playback
and other GPUs need separate measurements.

The layer comparisons include fresh input upload, the existing layer/scene/blit
passes and GPU completion, with no readback. Two preallocated source images
alternate to prevent the render engine from skipping unchanged-frame uploads.
All three layer paths use the same disabled color-management setting. The
previous path expands through the general format/channel CPU function; the
NEON comparison uses the existing optimized channel-only overload. The fused
path uploads the original packed data and supplies missing channels in the
layer shader. Pipeline compilation/setup are excluded by warmup.

At 4K, NEON expansion plus rendering measured **8.308 / 15.024 / 28.287 ms**
for UInt8 / Half / Float input. Fused native sampling measured
**3.392 / 5.753 / 10.177 ms**, respectively. The previous general CPU expansion
plus rendering measured **237.525 / 245.304 / 257.768 ms**. These are completed
full-layer measurements, unlike isolated CPU expansion timings.

Writer packing measured **1.243 / 4.264 ms** at 1080p / 4K, versus
**1.724 / 5.600 ms** for the completed Core Image baseline. Simple CPU-to-CPU
channel expansion still favors the existing NEON kernels. GPU residency and
avoiding extra transfers determine the appropriate path.

[Current machine-readable results](benchmarks/image-conversion-m1-refactor.json).
[Initial conversion benchmark](benchmarks/image-conversion-m1.json).

| Resolution | Operation | Path | Median | p95 estimate | Calling-thread CPU | Recording |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| 1920x1080 | `rgb8_to_rgba8` | `cpu_sdk_neon` | 0.181 | 0.183 | 0.181 | 0.000 |
| 1920x1080 | `rgb8_to_rgba8` | `gpu_upload_convert_resident_output` | 0.952 | 1.037 | 0.396 | 0.390 |
| 1920x1080 | `rgb8_to_rgba8` | `gpu_upload_convert_readback` | 1.945 | 1.997 | 1.096 | 0.397 |
| 1920x1080 | `layer_rgb8_to_rgba8` | `previous_cpu_general_then_layer` | 59.678 | 62.518 | 58.402 | 58.382 |
| 1920x1080 | `layer_rgb8_to_rgba8` | `cpu_neon_then_layer` | 2.411 | 2.522 | 1.353 | 1.337 |
| 1920x1080 | `layer_rgb8_to_rgba8` | `gpu_fused_native_layer` | 1.064 | 1.087 | 0.373 | 0.366 |
| 1920x1080 | `rgb_half_to_rgba_half` | `cpu_sdk_neon` | 0.481 | 0.496 | 0.481 | 0.000 |
| 1920x1080 | `rgb_half_to_rgba_half` | `gpu_upload_convert_resident_output` | 1.490 | 1.531 | 0.807 | 0.801 |
| 1920x1080 | `rgb_half_to_rgba_half` | `gpu_upload_convert_readback` | 3.704 | 3.785 | 2.289 | 0.880 |
| 1920x1080 | `layer_rgb_half_to_rgba_half` | `previous_cpu_general_then_layer` | 61.059 | 64.368 | 59.432 | 59.410 |
| 1920x1080 | `layer_rgb_half_to_rgba_half` | `cpu_neon_then_layer` | 4.251 | 4.397 | 2.710 | 2.689 |
| 1920x1080 | `layer_rgb_half_to_rgba_half` | `gpu_fused_native_layer` | 1.640 | 1.686 | 0.801 | 0.793 |
| 1920x1080 | `rgb_float_to_rgba_float` | `cpu_sdk_neon` | 0.966 | 0.996 | 0.968 | 0.000 |
| 1920x1080 | `rgb_float_to_rgba_float` | `gpu_upload_convert_resident_output` | 2.695 | 2.733 | 1.607 | 1.600 |
| 1920x1080 | `rgb_float_to_rgba_float` | `gpu_upload_convert_readback` | 7.062 | 7.749 | 4.468 | 1.693 |
| 1920x1080 | `layer_rgb_float_to_rgba_float` | `previous_cpu_general_then_layer` | 63.692 | 65.027 | 61.176 | 61.154 |
| 1920x1080 | `layer_rgb_float_to_rgba_float` | `cpu_neon_then_layer` | 7.582 | 7.833 | 5.117 | 5.092 |
| 1920x1080 | `layer_rgb_float_to_rgba_float` | `gpu_fused_native_layer` | 2.726 | 2.786 | 1.664 | 1.657 |
| 1920x1080 | `rgba_float_to_rgba_half` | `cpu_sdk_scalar` | 41.291 | 41.649 | 41.289 | 0.000 |
| 1920x1080 | `rgba_float_to_rgba_half` | `gpu_resident_texture` | 0.651 | 0.683 | 0.016 | 0.010 |
| 1920x1080 | `rgba_float_to_rgba_half` | `gpu_upload_convert_resident_output` | 3.423 | 3.720 | 2.188 | 2.181 |
| 1920x1080 | `rgba_float_to_rgba_half` | `gpu_upload_convert_readback` | 5.609 | 5.906 | 3.908 | 2.291 |
| 1920x1080 | `rgba8_to_bgra8` | `cpu_scalar_reference` | 2.729 | 2.739 | 2.730 | 0.000 |
| 1920x1080 | `rgba8_to_bgra8` | `cpu_neon_reference` | 0.349 | 0.353 | 0.349 | 0.000 |
| 1920x1080 | `rgba8_to_bgra8` | `qrhi_metal_encoder_surface` | 1.243 | 1.283 | 0.518 | 0.000 |
| 1920x1080 | `rgba8_to_bgra8` | `previous_coreimage_completed` | 1.724 | 2.038 | 0.845 | 0.000 |
| 3840x2160 | `rgb8_to_rgba8` | `cpu_sdk_neon` | 0.975 | 1.020 | 0.976 | 0.000 |
| 3840x2160 | `rgb8_to_rgba8` | `gpu_upload_convert_resident_output` | 3.052 | 3.121 | 1.633 | 1.626 |
| 3840x2160 | `rgb8_to_rgba8` | `gpu_upload_convert_readback` | 6.840 | 7.985 | 4.347 | 1.710 |
| 3840x2160 | `layer_rgb8_to_rgba8` | `previous_cpu_general_then_layer` | 237.525 | 241.883 | 234.485 | 234.473 |
| 3840x2160 | `layer_rgb8_to_rgba8` | `cpu_neon_then_layer` | 8.308 | 8.726 | 5.213 | 5.186 |
| 3840x2160 | `layer_rgb8_to_rgba8` | `gpu_fused_native_layer` | 3.392 | 3.448 | 1.666 | 1.655 |
| 3840x2160 | `rgb_half_to_rgba_half` | `cpu_sdk_neon` | 1.941 | 1.979 | 1.942 | 0.000 |
| 3840x2160 | `rgb_half_to_rgba_half` | `gpu_upload_convert_resident_output` | 5.283 | 5.481 | 3.309 | 3.302 |
| 3840x2160 | `rgb_half_to_rgba_half` | `gpu_upload_convert_readback` | 13.961 | 15.113 | 9.301 | 3.395 |
| 3840x2160 | `layer_rgb_half_to_rgba_half` | `previous_cpu_general_then_layer` | 245.304 | 256.724 | 240.847 | 240.840 |
| 3840x2160 | `layer_rgb_half_to_rgba_half` | `cpu_neon_then_layer` | 15.024 | 20.087 | 10.333 | 10.303 |
| 3840x2160 | `layer_rgb_half_to_rgba_half` | `gpu_fused_native_layer` | 5.753 | 5.951 | 3.350 | 3.343 |
| 3840x2160 | `rgb_float_to_rgba_float` | `cpu_sdk_neon` | 3.923 | 4.239 | 3.924 | 0.000 |
| 3840x2160 | `rgb_float_to_rgba_float` | `gpu_upload_convert_resident_output` | 10.019 | 10.144 | 6.679 | 6.671 |
| 3840x2160 | `rgb_float_to_rgba_float` | `gpu_upload_convert_readback` | 30.134 | 37.630 | 19.127 | 6.782 |
| 3840x2160 | `layer_rgb_float_to_rgba_float` | `previous_cpu_general_then_layer` | 257.768 | 264.106 | 250.299 | 250.278 |
| 3840x2160 | `layer_rgb_float_to_rgba_float` | `cpu_neon_then_layer` | 28.287 | 30.569 | 19.916 | 19.891 |
| 3840x2160 | `layer_rgb_float_to_rgba_float` | `gpu_fused_native_layer` | 10.177 | 12.323 | 6.874 | 6.867 |
| 3840x2160 | `rgba_float_to_rgba_half` | `cpu_sdk_scalar` | 165.709 | 169.570 | 165.689 | 0.000 |
| 3840x2160 | `rgba_float_to_rgba_half` | `gpu_resident_texture` | 1.520 | 1.544 | 0.019 | 0.012 |
| 3840x2160 | `rgba_float_to_rgba_half` | `gpu_upload_convert_resident_output` | 12.331 | 12.880 | 8.910 | 8.903 |
| 3840x2160 | `rgba_float_to_rgba_half` | `gpu_upload_convert_readback` | 20.841 | 24.579 | 14.999 | 9.314 |
| 3840x2160 | `rgba8_to_bgra8` | `cpu_scalar_reference` | 10.943 | 11.263 | 10.944 | 0.000 |
| 3840x2160 | `rgba8_to_bgra8` | `cpu_neon_reference` | 1.995 | 2.040 | 1.997 | 0.000 |
| 3840x2160 | `rgba8_to_bgra8` | `qrhi_metal_encoder_surface` | 4.264 | 4.358 | 2.221 | 0.000 |
| 3840x2160 | `rgba8_to_bgra8` | `previous_coreimage_completed` | 5.600 | 6.248 | 2.943 | 0.000 |

## Verification

`flipman.sdk.cpu_image_converter` uses a QCoreApplication with no GPU device.
It checks normalized values across all eleven scalar types, geometry and color
metadata, BGR/BGRA layout tags, odd widths, NEON tails and shared/detached
identity behavior. Existing ImageBuffer convenience calls are checked against
the facade.

`flipman.sdk.image_converter` compares CPU references with GPU readbacks for
RGB/RGBA/BGRA, UInt8/Half/Float, odd widths, nonzero image origins, alpha,
negative/HDR values, texture conversion and native IOSurface output.
`flipman.sdk.render_channels` checks fused native packed sampling against
expanded RGBA textures, 1/2/3/4-channel cache transitions and transitions back to
NV12/UYVY. `flipman.sdk.quicktime_writer` verifies generated codec round trips
and writer lifecycle behavior. Readbacks and CPU pixel comparisons are confined
to verification/export APIs rather than playback.

Both Debug and Release pass all 53 enabled CTest cases after this refactor,
with no media cases skipped. The legacy randomized timer case remains disabled.
Standalone public headers and the affected demo applications also build.
