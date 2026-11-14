# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Mikael Sundell

"""Application facade layered over the existing native Flipman SDK module."""
import contextlib
import io
import traceback
import flipman as sdk
import flipman_app as app

runtime = sdk.Runtime()  # Borrows the running SDK application.


class _Commands:
    def undo(self):
        app._undo()

    def redo(self):
        app._redo()


class _Track:
    def clips(self):
        return [_Clip()] if app._state()['loaded'] else []


class _Clip:
    def name(self):
        return app._state()['title']


class _Timeline:
    def fps(self):
        state = app._state()
        return sdk.Fps(state['fps_numerator'], state['fps_denominator'], state['drop_frame'])

    def time(self):
        return sdk.Time.fromFrames(app._state()['frame'], self.fps())

    def timeRange(self):
        return sdk.TimeRange(sdk.Time.zero(self.fps()), sdk.Time.fromFrames(app._state()['frame_count'], self.fps()))

    def tracks(self):
        return [_Track()] if app._state()['loaded'] else []

    def isPlaying(self):
        return app._state()['playing']

    def seek(self, time):
        app._set_frame(time.frames())

    def setFps(self, fps):
        app._set_fps(fps.numerator(), fps.denominator(), fps.dropFrame())

    def play(self):
        app._play()

    def stop(self):
        app._stop()


class _Session:
    def filename(self):
        return app._state()['filename']

    def isLoaded(self):
        return app._state()['loaded']

    def isLoading(self):
        return app._state()['loading']

    def openFile(self, filename):
        app._open_file(str(filename))

    def setFrame(self, frame):
        app._set_frame(frame)

    def setFps(self, fps):
        self.timeline().setFps(fps)

    def timeline(self):
        return _Timeline()

    def commands(self):
        return _Commands()

    def setDisplayProfile(self, name):
        """Select a supported native surface profile through command history."""
        app._set_display(name)


session = _Session()
app.session = lambda: session


def _run_script(source):
    output = io.StringIO()
    with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
        try:
            exec(compile(source, '<Flipman console>', 'exec'), globals(), globals())
        except BaseException:
            traceback.print_exc()
    return output.getvalue()


class _McpServer:
    def start(self):
        return app._start_mcp()

    def stop(self):
        app._stop_mcp()

    def isRunning(self):
        return app._application_info()['mcpRunning']

    def url(self):
        return app._application_info()['mcpUrl']


app.mcpServer = lambda: _McpServer()
app.applicationInfo = app._application_info


class _Settings:
    def value(self, key, default=None):
        return app._settings_get(key, default)

    def setValue(self, key, value):
        app._settings_set(key, value)

    def filename(self):
        return app._application_info()['settingsFilename']

    def isIsolated(self):
        return app._application_info()['isolated']


class _Console:
    def start(self):
        return app._console('start')

    def stop(self):
        app._console('stop')

    def isRunning(self):
        return app._console('running')

    def text(self):
        return app._console('text')


app.settings = lambda: _Settings()
app.console = lambda: _Console()
