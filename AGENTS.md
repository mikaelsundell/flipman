# Flipman development guide

Adapted from the applicable C++ and build conventions in
[Stageviz](https://github.com/mikaelsundell/stageviz/blob/master/AGENTS.md).
Follow nearby code when it establishes a more specific pattern.

## SDK structure and ownership

- Keep reusable functionality in `sources/flipmansdk`; applications and demos
  should consume the public headers under `include/flipmansdk`.
- Keep public interfaces small. Use the existing PIMPL pattern for substantial
  classes, with private state in a `Data` struct. Access that state through
  `p->d` in the public wrapper and `d` in the private implementation.
- Organize private declarations as lifecycle/initialization, utilities,
  slots/actions, accessors, then the `Data` struct and object references.
- Document QObject ownership, borrowed pointers, shared data, and thread
  requirements explicitly. Do not assume that explicitly shared data detaches
  automatically or that a copied handle is an independent resource.
- Preserve public names and signatures during formatting-only changes. Keep
  behavior fixes separate and cover them with focused regressions.
- Do not put required work inside `Q_ASSERT`; assertions may be compiled out.

## C++ headers and formatting

- Use `.clang-format` (four-space indentation, 120 columns, C++17, sorted
  includes). Format only the files relevant to the change.
- Public headers must compile on their own. Include the Qt and standard
  library declarations they use rather than relying on include order.
- Use `#pragma once`. Keep generated export headers in the build directory.
- Write API documentation as multiline Doxygen blocks immediately before the
  declaration, including enum values and members. Do not use compact
  `/** @brief ... */` blocks or trailing `///<` comments.
- Put `@brief` on its own documentation line. Leave a blank documentation line
  before parameter/return tags. Keep grouping directives in multiline blocks.
- Describe implemented behavior accurately; label incomplete APIs rather than
  implying that a declared feature already works.

## Builds and verification

- Prefer `build_app.sh debug|release` for configuration-specific Xcode builds
  under `build.debug`/`build.release`. See `docs/build-deploy.md` for packaging.
  Keep Blackmagic RAW runtime staging separate from the installed DeckLink
  Desktop Video driver; see `docs/blackmagic-runtime.md`.
- Use out-of-source builds and an explicit dependency prefix; do not add
  machine-specific SDK or third-party paths to source or scripts.
- For the macOS Debug dependency environment, prefix configure, build, and run
  commands with `DYLD_IMAGE_SUFFIX=_debug`. Do not apply that suffix to Release
  dependencies. Use matching Qt libraries, tools, and plugins.
- Compile the `flipmansdk_headers` target after changing public headers. It
  includes each header in a separate translation unit using consumer settings.
- Build the affected SDK targets and run relevant regressions. Report exactly
  what was verified and any remaining build or runtime limitations.
- Keep automated tests distinct from interactive demos and hardware checks.
  Register automated cases individually with CTest, labels, and bounded
  timeouts. Prefer deterministic fixtures and temporary output directories.
- Keep GPU/hardware integration tests separate from CPU-only correctness
  checks. Do not use timing thresholds as general correctness assertions.

## Python bindings

- Keep SDK bindings independent of the application window so the same native
  API can later be exposed by an embedded interpreter.
- Document the Python API actually exposed, including argument names,
  keyword support, ownership, errors, and asynchronous completion.
- Update reference documentation and Python behavior tests with each binding
  change. Documentation checks supplement behavior tests.
- Keep `docs/python-sdk.md` synchronized with the exposed module. Run the
  relevant `flipman.python.*` CTest cases. Use the generated `flipman-python`
  launcher for manual runs; do not use `/usr/bin/python3` with Debug Qt, since
  the protected macOS launcher may strip `DYLD_IMAGE_SUFFIX`.
- Define Qt application/thread ownership and interpreter lifetime explicitly.
  Do not retain borrowed native objects without a lifetime guard.
- Compile Python/Qt binding targets with `QT_NO_KEYWORDS`, as Stageviz does, so
  Qt's `slots` macro cannot collide with CPython headers. Use `Q_SIGNALS`,
  `Q_SLOTS` and `Q_EMIT` in SDK headers.

## Profiling and diagnostics

- Use `FM_PROFILE_SCOPE` for aggregate/explicit recordings and `core::PerfScope`
  for optional JSONL phase timing. See `docs/profiling.md` for their independent
  controls. Keep instrumentation at actionable function or batch boundaries.
- Use `qCDebug(core::mediaLog)` or `qCDebug(core::renderLog)` for routine SDK
  diagnostics. Categories default to warnings; do not introduce unconditional
  per-frame `qDebug()` output. Preserve warnings/errors for actual failures.
- Profile host work honestly: frame submission is not GPU completion, and open
  dispatch is not asynchronous completion. Do not use profiler gates to change
  normal media or rendering behavior.

## Playback performance

- Keep channel reordering, color conversion and effect processing on the GPU.
  Do not add CPU per-pixel conversions, extra full-frame copies/uploads, readbacks
  or additional render passes to the playback path for these operations.
- Preserve native decoder layouts and normalize samples in the existing GPU
  pass. Keep CPU pixel comparisons and reference conversions in tests/offline
  tooling. Validate CPU/GPU uniform sizes and offsets explicitly.

## Application work

- Keep substantial Qt interfaces in `.ui` files and let `uic` generate wrappers.
  Store widget tooltips in the designer file.
- Derive UI state from the SDK/controller instead of maintaining a second,
  unsynchronized copy. Verify observable state and signals in tests.

## Repository hygiene

- Preserve unrelated user changes. Keep generated files and test outputs out
  of the source tree.
- Never publish sensitive local data, paths, logs, captures, or credentials.
- When asked to commit, use `git commit --signoff` with the contributor's Git
  identity. Use a short lowercase type prefix and a concise summary; add
  subject-led bullets when useful. Do not certify another author's work.
