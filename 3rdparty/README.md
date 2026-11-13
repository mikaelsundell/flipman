# Vendored dependencies

## glsl-parser

- Upstream: https://github.com/graphitemaster/glsl-parser
- Revision: `8fd08ed39973b268953cb4a7100097613e0134a0` (main, retrieved 2026-11-08).
- Copyright: 2014 Dale Weiler.
- License: [MIT](glsl-parser/LICENSE), retained verbatim. Include this notice in
  binary distributions containing the SDK as well as source distributions.
- The complete upstream source snapshot, examples and tests are retained under
  `glsl-parser/`; no nested Git repository or build-time download is needed.
- Flipman's CMake adapter builds only `ast.cpp`, `lexer.cpp`, `parser.cpp` and
  `util.cpp` into a private static dependency of the SDK. Upstream's CLI and
  Python test driver remain available but are not used by Flipman's build.

Local patches (marked `Flipman` where practical):

- Read-only token inspection and explicit token cleanup for reflection.
- Release the current parser token when parsing exits with an error.
- Accept `const` function parameters; reject missing commas, unknown parameter
  types, named `void` parameters and trailing commas; advance array brackets
  correctly, including multidimensional arrays.
- Use bounded `vsnprintf` for diagnostic formatting.
- Range-check `#version` values before narrowing them to an integer.
- Fix the vector wrapper's empty/end pointers, `back()` and erase return values.
  Upstream dereferenced the one-past-end element; Xcode Debug's libc++ hardening
  aborts on that operation. `flipman.sdk.glsl_container` exercises these boundaries
  with hardening enabled.

Keep these changes when updating the pinned snapshot, and run
`flipman.sdk.shader_parser` plus the shader compilation/render regressions.

## How Flipman uses the parser

Flipman expands its `@param`, `@include` and template injection syntax first.
The upstream lexer scans the expanded GLSL. At top-level function definitions,
Flipman parses the declaration as an upstream prototype AST and copies its
return/parameter types into `ShaderFunction`. Prototypes alone do not satisfy
entry contracts; comments, function calls and nested statements are not reflected.
Multiline signatures and precision/storage qualifiers no longer depend on regex
matching. Delimiter and lexer errors are reported by `ShaderParser`.

This is **declaration reflection, not complete GLSL validation**. Function bodies
are scanned for balanced delimiters, but their expressions, types and semantics
are validated by Qt's shader compiler after all generated uniforms and helper
functions have been assembled. Arbitrary global declarations are not validated
by reflection either. Do not treat a successful `ShaderParser::parse` as proof
that a shader will compile.

Upstream does not implement a general GLSL preprocessor or all modern GLSL
features. `#version` and `#extension` are supported; other `#` directives produce
an error. Function signatures using user-defined struct types or unsized arrays
are currently unsupported. Built-in functions/variables need no synthetic
stand-ins because function bodies are left to the compiler.

## ACES Studio OCIO config

Flipman embeds the unmodified
`studio-config-v4.0.0_aces-v2.0_ocio-v2.5.ocio` snapshot in SDK resources.
Upstream: https://github.com/AcademySoftwareFoundation/OpenColorIO-Config-ACES.
The BSD-3-Clause license is retained beside the config at
[`resources/configs/LICENSE`](../sources/flipmansdk/resources/configs/LICENSE)
and included in packaged app notices. OCIO 2.5+ implements its built-in transforms.

## UI resources

The UI theme, icons and timeline ruler retain their BSD-3-Clause copyright
notices. `Application` owns the UI service, accessed through `core::ui()` by widgets
and render code. All UI resources are registered by `resources/ui/ui.qrc`.
Generic widget/image-editor icons are retained; USD/3D composition and shading
icons are excluded. Application artwork lives under `sources/flipman/resources`.
The Transform icon role maps to Move.

`render::DisplayColorSpace` owns display-profile and cached ICC data, shared by
UI defaults and independently selected viewer tags. The timeline ruler lives in
`widgets/timelineslider.*`. Explicit gamut/curve labels describe the canonical
OCIO identifiers used for processing.
