# Flipman application

The `flipman` target builds the main macOS application. `testviewer` remains a
small SDK test harness. The application opens movies and images through the SDK,
with native Metal presentation and the same GPU color pipeline as the harness.

## Layout and ownership

`app/viewer.ui` defines the main window. Separate Designer layouts keep the
surfaces independent:

- `PlaybackView`: open, transport, loop, in/out points, source timecode, frame,
  FPS, drop-frame notation, every-frame playback and fullscreen.
- `TimelineView`: adaptive `widgets::ScrubLine`, in/out labels and frame count.
- `DisplayView`: input, working and output encodings, viewing transform and the
  independent macOS display profile. It sits directly below the scrub line.
- `FileSystemView`: directory browser on the left; double-click a file to open it.
- `RenderView`: SDK Metal viewer, fit/zoom, project resolution and background menu.
- `MetadataView`: source information and grouped decoder metadata on the right.

The left and right panels collapse by dragging the splitter. The View menu can
restore them. Window geometry, splitter sizes and recent files use the application
Settings service, backed by QSettings.
The viewport uses the shared SDK UI theme and resources.

`flipman::Application` derives from the SDK application and owns Session,
PythonInterpreter, Settings, Console and the optional McpServer. It reuses the
SDK-owned UI, plugin registry and native services. Application-thread accessors
`app()`, `session()`, `pythonInterpreter()`, `settings()`, `console()`, `mcpServer()`
and `ui()` return borrowed subsystems. Views obtain the global session when
constructed and observe its specific changed signals. Playback/range signals
are forwarded by Session, so views do not reconnect to individual timelines.

`flipman::Session` owns the SDK timeline,
renderer and command history. Opening or dropping a file validates its first
frame on a dedicated decoder worker, then installs a fresh timeline containing
one track, one clip and the media handle. A failed open preserves the previous
document. Superseded requests cannot replace its timeline or decoder. The user
sees a file player; the underlying composition model remains explicit.

The session and views run on the application thread. Reader open, seek, read and
close operations are serialized on one worker. QuickTime close/destruction cancels
and drains outstanding AVFoundation open callbacks before releasing reader state;
queued notifications from an older open are ignored. Playback retains native decoded
image storage and uses the existing GPU shader path for pixel sampling, effects
and color conversion. This controller adds no CPU pixel transforms or readbacks.
Normal playback advances against a monotonic clock and may skip presentation
frames when decoding falls behind. Every-frame playback waits for each decoded
frame, so it can run slower than real time. Both respect the in/out range and loop.

SDK `av::Timeline` stores composition, time, FPS and playback state; the application
session schedules decoding and presentation. Renderer GPU allocations are released
when a viewport detaches or its device is retired; layers and color settings stay
in the session so another viewport can initialize from the same document. FPS changes preserve frame indices
and clip length, changing playback duration. The source timecode offset remains
separate from the zero-based timeline cursor.

## Commands

The session's bounded command stack supports execute, undo, redo and changes
already applied during a scrub. Time, FPS, color transforms and display-profile
changes use this same stack from UI and Python. A scrub drag creates one undo
entry; ordinary playback ticks create none. Opening or closing a document clears
history. Loop, in/out markers, background and viewport navigation currently apply
directly and are not undoable.

## Python interpreter

Choose **View → Python Interpreter**. The console embeds CPython and imports the
existing native nanobind `flipman` SDK module as `sdk`; it does not launch a
separate process. It initializes lazily and retains variables between runs.
The session facade uses guarded references to the live application session:

```python
print(session.filename())
print(session.timeline().fps().real())
print(session.timeline().time().frames())  # zero-based
session.setFrame(10)
session.setFps(sdk.Fps(25, 1))
session.setDisplayProfile("sRGB")
session.commands().undo()
```

`import flipman_app as app; app.session()` returns the same facade. It offers
`filename`, `isLoaded`, `isLoading`, `openFile`, `setFrame`, `setFps`,
`setDisplayProfile`, `timeline` and `commands`. Timeline inspection provides
`fps`, `time`, `timeRange`, `tracks`, `isPlaying`, plus `seek`, `setFps`, `play`
and `stop`. Track/clip proxies currently expose only `clips()` and `name()`.
These are application proxies, not additional SDK nanobind types.

The same module also provides:

```python
app.settings().setValue("viewer/example", ["hello", True, 42])
print(app.settings().value("viewer/example"))
print(app.settings().filename(), app.settings().isIsolated())
print(app.applicationInfo())
app.mcpServer().start()
print(app.mcpServer().url())
app.mcpServer().stop()
app.quit()
```

`settings().value(key, default=None)` and `setValue(key, value)` support finite JSON
values, signed 64-bit integers and bytes (for native geometry/state). Unsupported
Python objects raise TypeError; oversized integers raise OverflowError. Reads of
Qt-only values without a JSON representation return None. Preference writes flush
immediately. `app.console()` offers `start()`, `stop()`, `isRunning()` and `text()`.
It captures native stdout/stderr rather than forwarding it to the launch terminal;
Python script output is still returned to the interpreter dialog or MCP caller.
All facade methods require the application thread. The application stops MCP,
finalizes Python while session/settings services are alive, then releases them.


`openFile` is asynchronous; inspect `isLoading()` after returning to the event
loop. Frame requests update the timeline immediately and decode asynchronously.
Scripts run on the GUI thread and pause playback before execution; long scripts
will block interaction. The GIL is released between scripts so Python worker
threads can run, but session calls must stay on the GUI thread. Python errors and printed output appear in the console.
No Qt wrapper objects are exposed. See [SDK Python API](python-sdk.md) for native
SDK types and ownership rules.

## Development builds and limits

`FLIPMAN_BUILD_APP=ON` is the default. The default `FLIPMAN_APP_PYTHON=ON` requires
matching CPython embedding headers/library and enables SDK Python bindings.
Use `FLIPMAN_APP_PYTHON=OFF` for an application without the interpreter, or
`FLIPMAN_BUILD_APP=OFF` for SDK-only builds.

On macOS Debug, always set this environment variable **before process startup**:

```ini
DYLD_IMAGE_SUFFIX=_debug
```

Generated Debug Xcode executable schemes include it automatically. For shell
launches, prefix the command with `DYLD_IMAGE_SUFFIX=_debug`.
The executable is `build.debug/sources/flipman/Debug/flipman.app` (or the Release
equivalent). It accepts an optional movie/image path. The interpreter currently
uses the Python home and extension directory selected by CMake. A self-contained
Python runtime/stdlib deployment still needs packaging work; this is a working
development application, not a verified standalone distribution.

This first application pass supports video/image playback. Audio output,
multi-clip editing and session-file persistence are not implemented.
Application CTest cases cover composition creation, seeks, failed/superseded
opens, playback, history, Python and collapsible viewer panels using temporary
fixtures and isolated preferences.

## Clean runs and local MCP control

A clean run starts with fresh preferences without reading or changing normal
user settings. Both environment and CLI paths use a separate `DIR/PID/flipman.ini`
for each process, with QSettings fallbacks disabled:

```sh
DYLD_IMAGE_SUFFIX=_debug FLIPMAN_TEST_SETTINGS_DIR=/tmp/flipman-review \
  build.debug/sources/flipman/Debug/flipman.app/Contents/MacOS/flipman --mcp
```

`--settings-dir DIR` overrides the environment root. `--clean-run` without either
root creates a temporary directory and removes it on orderly exit. An explicit
root retains its per-process files for inspection. No normal geometry, recent
files or persisted MCP-enabled preference is inherited. The app honors these
settings before creating Session or any views. Tests use the same isolation path.

The optional endpoint starts through `--mcp`, **View → MCP server**, or
`app.mcpServer().start()` in the interpreter. The menu preference is remembered
for normal runs. It binds only to IPv4 loopback, at
`http://127.0.0.1:37652/mcp` by default. `--mcp-port PORT` overrides the port for
this process; port 0 selects an available port. Test controllers may also use
`FLIPMAN_TEST_MCP_PORT`. Startup prints the selected URL unless console capture
is active (`--console`). MCP remains idle when stopped.

The endpoint supports stateless MCP JSON-RPC over HTTP POST: initialize, ping,
notifications, tools/list and tools/call. It returns JSON responses without an
SSE stream. Foreign Host/Origin values and oversized requests are rejected.
Tools are:

- `flipman_get_session_info`, `flipman_get_metadata`.
- `flipman_open_file`, `flipman_close_file`.
- `flipman_set_time` (zero-based `frame`), `flipman_set_fps` (integer `numerator`,
  optional `denominator=1`, `dropFrame=false`).
- `flipman_set_display_profile` (`name`; the tool schema lists supported tags).
- `flipman_play`, `flipman_stop`, `flipman_undo`, `flipman_redo`.
- `flipman_run_python` (`code`, at most 64 KiB) and `flipman_quit`.

File opening and image presentation remain asynchronous. Poll session info for
`loading`, `loaded`, `frame` and `presentedFrame`. Native time/FPS/display tools
use the same undo stack as the UI; Python session facade edits do too. Python
runs synchronously on the GUI thread with the permissions of the desktop
process, and can block interaction. Quit is scheduled after the HTTP response.

`--execute CODE` runs in the shared interpreter after startup, or after an
explicitly requested file opens. `--open FILE` and a positional filename are
supported. Native file-open events use Application's `fileOpenRequested` signal.

The clean-run regression launches the real executable in several isolated
processes, exercises MCP and Python preferences, verifies that processes cannot
inherit each other's settings, and checks orderly Python/settings teardown.
