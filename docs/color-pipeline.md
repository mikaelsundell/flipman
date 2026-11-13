# GPU color management

Flipman embeds the unmodified Studio v4.0.0 / ACES 2.0 / OCIO 2.5 config.
OpenColorIO 2.5 or newer is a build dependency. The config and
its upstream license are under `sources/flipmansdk/resources/configs`; packaging
includes the license in the app's notices. No source checkout is needed at runtime.

## Rendering

SDK color management is opt-in through `RenderEngine::setColorSettings()`.
Existing SDK consumers retain their previous unconverted behavior; `testviewer`
enables management by default. The default working space is scene-linear ACEScg. Other scene-referred encodings,
including ARRI LogC3 / AWG3, may be selected for effects.

1. Decode/upload source pixels in their existing layout.
2. Normalize BGRA or supported YCbCr on the GPU.
3. Apply the layer's OCIO input-to-working transform in the existing layer shader.
4. Run the layer effect in the working space and composite into RGBA16F.
5. Apply the selected OCIO display/view once in the existing final blit,
   unpremultiplying/re-premultiplying RGB around the nonlinear transform.
6. Present through the independently tagged Metal layer.

No color-management pass, CPU per-pixel processing, image readback, or additional
full-frame upload is added to playback. OCIO prepares shader code and LUT data
on the CPU when settings change. The renderer caches generated shaders/pipelines
and GPU LUT resources. LUTs upload once per transform/device; shader math and LUT
sampling still have a GPU cost. Device recreation regenerates device resources.

`ImageLayer::setInputColorSpace()` accepts an exact config color-space name.
An empty override uses supported ImageBuffer tags. Unknown tags fail explicitly;
the SDK does not silently guess a camera gamut. `ColorPipeline` lists config
spaces, displays and views. Configs are cached by absolute filename for the
process lifetime; restart to pick up edits to a loaded external config.
An external config's relative LUTs resolve beside that config.

`ColorSettings` selects enabled/configFile/workingSpace/display/view. The working
space must be a non-data scene-referred config space. It may be linear or encoded.
The path is source → selected working encoding → effect → output; no extra
ACEScg conversion is inserted after the effect. Existing layer blending also
operates in that selected encoding. Correct linear-light blending for encoded
working spaces is deferred; use a linear working space when compositing matters.
Disabled SDK settings bypass color management. Dynamic OCIO uniforms are currently rejected; the supported config
transforms are static. All outputs on an engine currently share its output
encoding/view. Configure separate engines when outputs need different views.

## Viewing choices

**No tone mapping** is the default view label (OCIO identifier `Un-tone-mapped`). With the bundled config's un-tone-mapped
scene/display connection this preserves finished SDR video when input and output
encodings match. Choose an `ACES 2.0` view explicitly for scene-linear/camera-log
material that needs ACES rendering. Do not apply an ACES view indiscriminately to
already graded movies or film-rendered output.

## testviewer

Input, Working and Output use paired gamut/encoding controls. Their menus group
exact profiles by OCIO family, gamut and encoding. Explicit labels separate the
primaries from the encoding: ARRI Wide Gamut 3 offers Linear or LogC3 (EI800), for
example. The embedded config is extended in memory with independent pairs of its linear RGB
primaries and known decode curves, including AWG3 + Gamma 2.4. Original profiles
retain their identifiers. Curve-only OCIO transforms and linear gamut matrices
are composed once and optimized into the existing GPU shaders. External configs
retain only their explicitly defined pairs. The visible labels omit `- Display`, while
processing, persistence and tooltips retain the exact OCIO identifiers.

Each gamut and gamma selector offers Auto, a divider, grouped options, then a divider and Raw. Input Raw
bypasses source interpretation: input pixels are assumed to already be in the
selected working encoding. It does not bypass the effect or output view.
Output Raw and View Raw bypass the complete final output transform. Working Raw
inherits the resolved source encoding, so effects receive unchanged source values;
output conversion is told that encoding. A gamma selector's Raw bypasses only the
curve (linear values in the selected gamut). Auto Working resolves to ACEScg;
Auto Output resolves to Rec.1886 Rec.709, and Auto View to No tone mapping.
Input Auto resolves supported
source tags. Untagged ordinary stills default to sRGB display, movies to Rec.1886
Rec.709, and EXR/HDR to linear Rec.709; these are visible assumptions, not reliable
inference from file extensions. Override them for other material. Supported BRAW
decode gamma/gamut and color-science generation metadata can identify Film Gen5; otherwise choose Input.

Output selects an OCIO display encoding; View selects its rendering transform.
Working selects the encoding received by the `.fx` shader, defaulting to ACEScg.
Viewer color management is always enabled; there is no bypass checkbox. Input,
Working and Output use fixed-width gamut/gamma stacks aligned in a grid, with View beside them. A flexible spacer
separates these controls from the Display selector on the right.

Display tag remains the existing independent control. Changing Output syncs it only
for exact supported SDR mappings: sRGB, Gamma 2.2 Rec.709, Rec.1886 Rec.709, and
Display P3. Unmatched outputs leave the tag untouched. Users can override the tag
after a match; changing only View does not overwrite it. No new HDR/EDR tagging or
surface support is added. An OCIO HDR entry alone does not make the viewer HDR.

## Validation

`flipman.sdk.color_pipeline` compares GPU readback with OCIO CPU reference math
for ACEScg, AP0, ARRI log and display-referred inputs, SDR ACES 2.0 views, output
switches, an effect operating directly in LogC3/AWG3, and invalid-setting recovery. CPU reference conversion and GPU readback
are test-only operations. `flipman.python.render` exercises config discovery and
managed GPU rendering through the Python API. `flipman.viewer.drop` covers reloads.


## Display profiles

`render::DisplayColorSpace` is the SDK facade over QColorSpace. It accepts Qt
profiles, ICC data or a `DisplayTransform` descriptor and prepares ICC bytes when
constructed. `qtColorSpace()` provides explicit Qt integration; `applyTo()` sets
only the profile on a supplied QSurfaceFormat and never changes global state.
Unsupported display descriptors preserve the existing SDR fallback behavior.

`core::UI::colorSpace()` returns the application default profile. Updating it
applies the color-space field to the default QSurfaceFormat for subsequently
created surfaces while preserving other format fields. Widget code accesses the
application-owned service through `core::ui()`.

Viewer owns an independent display profile, exposed by `displayColorSpace()` and
selected through `setDisplayTransform()`. Its gamut/curve mapping is implemented
in the render facade. The Metal bridge caches a native CGColorSpace per viewer,
rebuilding it only after a profile change; initialization and resize reuse it.
Changing the UI default never overwrites the viewer tag. The facade adds no CPU
pixel processing or render pass. OCIO pixel transforms remain in the GPU pipeline.

## Viewer layout

SDK UI assets and the stylesheet live under `resources/ui`, registered by `ui.qrc`.
The test app uses a single `sources/tests/testviewer/viewer.ui`. The central
`MainWidget` supplies the base background. A `DockWidget` block holds
the color selectors. A separate block below contains zoom controls and the
Project resolution selector on the right; a separate render-view region contains the
Metal viewport and its own fixed 44-pixel bottom scrubber panel. Margins and spacing
come from Designer. Project Auto follows the decoded source display window; fixed presets fit the clip
inside that canvas. The scene target is rebuilt only when project resolution changes.
Uncovered project pixels remain transparent until the final presentation, where
hardware blending composites them over the viewer's black background after the
color transform. No additional pass or CPU pixel work is added.

The SMPTE field uses `Media::start()` plus the selected frame offset. Seeking still
uses `Media::timeRange()`; embedded source timecode does not shift decode positions.
QuickTime uses the timecode track's explicit drop-frame flag; a fractional video
rate alone does not make a clip drop-frame (23.976 NDF retains `:` separators).
Fit refreshes after drops/project changes, including when Fit is already selected,
and after the native render target is first created or resized.

The viewer's render-area context menu exposes Timeline background presets and a
custom color picker. The default is black; the chosen display background is used
both outside the canvas and for its uncovered pixels, after color processing.
The window title prefers the media's nonempty title metadata, then name/container
title, falling back to the filename. Auto gamma labels include the resolved curve.
