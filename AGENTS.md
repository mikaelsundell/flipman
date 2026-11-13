# Flipman development guide

C++ and build conventions for the Flipman SDK and applications.
Follow nearby code when it establishes a more specific pattern.

## SDK structure and ownership

- Keep reusable functionality in `sources/flipmansdk`; applications and demos
  should consume the public headers under `include/flipmansdk`.
- Keep public interfaces small. Use the existing PIMPL pattern for substantial
  classes, with private state in a `Data` struct. Access that state through
  `p->d` in the public wrapper and `d` in the private implementation.
- Keep all per-instance state in the private implementation's `Data`, including
  widget/object references, pointers, flags, caches and owned resources. Do not
  declare additional state beside `Data d;`: this avoids member-name clashes
  with implementation methods and inherited Qt APIs. For example, keep
  `QPointer<TimeEdit> widget;` inside `Data` and use `d.widget` / `p->d.widget`.
- Keep `Data` compact: no blank lines between member declarations, and no blank
  line between its closing `};` and `Data d;`. Group members by type (native
  handles, SDK values, Qt values, standard-library resources and scalar values),
  keeping like types together. The borrowed pointer to the owning widget/object
  is always the last member. Preserve declaration-dependent initialization and
  resource destruction order when grouping; inspect constructors and cleanup
  paths instead of blindly alphabetizing members.
- Organize private declarations as lifecycle/initialization, utilities,
  slots/actions, accessors, then the `Data` struct and its `Data d;` instance.
- Document QObject ownership, borrowed pointers, shared data, and thread
  requirements explicitly. Do not assume that explicitly shared data detaches
  automatically or that a copied handle is an independent resource.
- Preserve public names and signatures during formatting-only changes. Keep
  behavior fixes separate and cover them with focused regressions.
- Do not put required work inside `Q_ASSERT`; assertions may be compiled out.

## C++ headers and formatting

- Use `.clang-format` (four-space indentation, 120 columns, C++17, sorted
  includes). Format only the files relevant to the change.
- Use clang-format 21 or newer through `scripts/clang-format.sh` or `.bat`.
  Leave one blank line between function declarations, between function
  definitions, and between separate class or struct definitions. This includes
  constructors, destructors and overloads. When the next function has a Doxygen
  block, put the blank line before that block, keeping the documentation directly
  attached to its declaration. Leave a blank line inside namespace boundaries.
- Within functions, separate logical phases with a blank line: initial guards,
  setup, processing and result handling. Separate early-return branches from
  the next independent operation. Keep closely related statements together;
  do not insert a blank line after every statement.
- Leave a blank line after the file's copyright/license comment, around
  `#pragma once`, and between the include section and declarations.
- Put quoted local/private/generated headers first, followed by one blank line
  and a single uninterrupted block of angle-bracket includes. Within that block,
  order public `<flipmansdk/...>` headers, external libraries/platform headers,
  Qt (including `rhi`), then C++ standard headers; sort each category internally.
  Do not insert blank lines between angle-bracket categories. Keep conditional
  or platform includes within their preprocessor guards.
- Put internal SDK headers in the owning component's `private/` directory
  (for example `render/private/ociogpu.h`); include them as
  `"private/ociogpu.h"` from that component. Keep them out of the public include
  tree and install interface. Create private directories as headers need them.
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
- Use `test_app.sh debug|release|all` (or `test_app.bat`) for complete regression
  runs from configured builds. It refreshes CMake, builds `flipman_tests` and
  records CTest/JSON/JUnit results. `--no-build` is only for current binaries;
  use `--fail-on-skip` when enabled media cases must all execute. See
  `CTest/README.md` for filters and report semantics.
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

- Before writing or explaining Python usage, read `docs/python-sdk.md`. Match
  the exposed bindings and their argument casing rather than assuming every
  similarly named C++ method is available.
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
- Compile Python/Qt binding targets with `QT_NO_KEYWORDS` so
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

## Image conversion

- Keep portable scalar/NEON kernels in `core/imageconversion.cpp`, behind the
  private core header. `ImageBuffer::convert` remains a convenience wrapper;
  core CPU storage must not depend on rendering device initialization.
- Use `render::ImageConverter::convertCpu` for explicit CPU output and `record`
  for GPU commands. Keep QRhi implementation details in `RhiImageConverter`
  and native encoder surfaces in small platform adapters using composition.
- Choose by source/destination residency and required output, not just OS.
  CPU conversion is available on all platforms; QRhi uses the caller's Metal,
  Direct3D or other supported device. Do not silently fall back from GPU
  recording to CPU work or add implicit readbacks.
- Share packed-pixel shader sampling between the converter and layer shader.
  Benchmark completed work with upload/readback costs stated, and compare the
  full affected rendering path as well as isolated conversion kernels.

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
- Set icons through `core::ui()` and `core::UI::IconRole`. Keep resource paths
  and icon styling centralized in `UI` instead of duplicating them in widgets.
- Derive UI state from the SDK/controller instead of maintaining a second,
  unsynchronized copy. Verify observable state and signals in tests.
- Give controls stable `objectName` values for observable UI regressions.
  Inspect values, enabled state, labels, actions and signals after event
  processing; pair native viewer screenshots with SDK state assertions. Keep
  screenshots and GPU readbacks in verification code, outside playback.
- Keep plain snapshot values instead of retaining borrowed Qt wrappers across
  event processing. Exercise temporary fixtures in a disposable app process;
  do not replace or save a user session during automated verification.

## Repository hygiene

- Preserve unrelated user changes. Keep generated files and test outputs out
  of the source tree.
- Keep `investigation/` ignored. Never stage, force-add, copy into tracked
  files, package, or publish its contents. Check the staged file list before
  committing; write reusable fixtures independently of local investigation data.
- Never publish sensitive local data, paths, logs, captures, or credentials.
- When asked to commit, use `git commit --signoff` with the contributor's Git
  identity. Use a short lowercase type prefix and a concise summary; add
  subject-led bullets when useful. Do not certify another author's work.

## Pull request preparation

- Inspect all commits introduced by a PR for DCO `Signed-off-by:` trailers
  matching their authors. Signing off the latest commit does not cover earlier
  commits. An `Authorized-by:` trailer does not replace a sign-off.
- Preserve other contributors' identities and do not certify their work. Only
  rewrite or push history when authorized; use an explicit `--force-with-lease`
  guard for an authorized history rewrite and verify the source tree is unchanged.
- Check required CI, DCO and regression results before declaring a PR ready.
  Report pending checks and anything that could not be verified.
