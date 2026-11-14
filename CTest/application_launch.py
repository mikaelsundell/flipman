# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Mikael Sundell

"""Exercise the real executable with disposable settings and a private MCP port."""

import json
import os
from pathlib import Path
import queue
import subprocess
import sys
import tempfile
import threading
import time
import urllib.request


def rpc(url, method, params=None):
    payload = dict(jsonrpc='2.0', id=1, method=method)
    if params is not None:
        payload['params'] = params
    request = urllib.request.Request(url, json.dumps(payload).encode(),
                                     {'Content-Type': 'application/json'})
    with urllib.request.urlopen(request, timeout=15) as response:
        return json.load(response)['result']


def tool(url, name, arguments=None):
    result = rpc(url, 'tools/call', dict(name=name, arguments=arguments or {}))
    if result.get('isError'):
        raise AssertionError(result)
    return result['content'][0]['text']


def launch(binary, configuration, root, options, marker, capture=False):
    environment = dict(os.environ)
    if sys.platform == 'darwin' and configuration == 'Debug':
        environment['DYLD_IMAGE_SUFFIX'] = '_debug'
    if root is None:
        environment.pop('FLIPMAN_TEST_SETTINGS_DIR', None)
    else:
        environment['FLIPMAN_TEST_SETTINGS_DIR'] = str(root)
    environment.pop('FLIPMAN_TEST_MCP_PORT', None)
    environment.pop('QT_QPA_PLATFORM', None)
    descriptor, ready_name = tempfile.mkstemp(prefix='flipman-ready-', suffix='.json')
    os.close(descriptor)
    ready = Path(ready_name)
    ready.unlink()
    environment['FLIPMAN_READY_FILE'] = str(ready)
    code = ("import json, atexit, os\nfrom pathlib import Path\n"
            "assert app.settings().isIsolated()\n"
            "assert app.settings().value('regression/marker') is None\n"
            "app.settings().setValue('regression/marker', %r)\n"
            "atexit.register(lambda: app.settings().setValue('regression/finalized', True))\n"
            "atexit.register(lambda: app.mcpServer().isRunning())\n"
            "Path(os.environ['FLIPMAN_READY_FILE']).write_text(json.dumps(app.applicationInfo()))\n"
            "print('FLIPMAN_READY ' + json.dumps(app.applicationInfo()))" % marker)
    command = [binary, '--mcp', '--mcp-port', '0', '--execute', code] + options
    if capture:
        command.append('--console')
    process = subprocess.Popen(command, env=environment, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True)
    lines = queue.Queue()

    def read_output():
        for line in process.stdout:
            lines.put(line)

    reader = threading.Thread(target=read_output, daemon=True)
    reader.start()
    output = []
    info = None
    try:
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if ready.is_file():
                try:
                    info = json.loads(ready.read_text())
                except json.JSONDecodeError:
                    continue
                break
            try:
                line = lines.get(timeout=.2)
            except queue.Empty:
                if process.poll() is not None:
                    break
                continue
            output.append(line)
            if line.startswith('FLIPMAN_READY '):
                info = json.loads(line[len('FLIPMAN_READY '):])
                break
        assert info is not None, ''.join(output)
        filename = Path(info['settingsFilename'])
        directory = Path(info['settingsDirectory'])
        assert info['isolated'] and filename.parent == directory
        assert directory.name == str(process.pid), info
        assert info['mcpRunning'], info
        state = json.loads(tool(info['mcpUrl'], 'flipman_get_session_info'))
        assert not state['loaded'] and state['settingsDirectory'] == str(directory), state
        result = tool(info['mcpUrl'], 'flipman_run_python', {'code':
            "assert app.settings().value('regression/marker') == %r\n"
            "app.settings().setValue('regression/values', ['hello', True, 42])\n"
            "assert app.settings().value('regression/values') == ['hello', True, 42]\n"
            "app.settings().setValue('regression/bytes', b'abc\\x00z')\n"
            "assert app.settings().value('regression/bytes') == b'abc\\x00z'\n"
            "assert app.console().isRunning() == %r\n"
            "print('shared services verified')" % (marker, capture)})
        assert 'shared services verified' in result, result
        tool(info['mcpUrl'], 'flipman_quit')
        assert process.wait(timeout=15) == 0, ''.join(output)
        if root is None and '--clean-run' in options:
            assert not directory.exists(), 'temporary clean-run directory was retained'
        else:
            assert filename.is_file(), 'isolated preferences were not written'
            assert 'finalized=true' in filename.read_text(), 'Python finalized after settings teardown'
        return info
    finally:
        ready.unlink(missing_ok=True)
        if process.poll() is None:
            process.kill()
            process.wait(timeout=10)
        reader.join(timeout=2)
        process.stdout.close()


def main():
    binary, configuration = sys.argv[1:3]
    with tempfile.TemporaryDirectory(prefix='flipman-app-launch-') as temporary:
        root = Path(temporary)
        first = launch(binary, configuration, root / 'settings', [], 'first')
        second = launch(binary, configuration, root / 'settings', [], 'second', capture=True)
        assert first['settingsDirectory'] != second['settingsDirectory']
        assert Path(first['settingsDirectory']).parent == root / 'settings'
        explicit = launch(binary, configuration, root / 'unused',
                          ['--settings-dir', str(root / 'explicit')], 'explicit')
        assert Path(explicit['settingsDirectory']).parent == root / 'explicit'
        assert not (root / 'unused').exists(), 'environment root overrode explicit settings'
        launch(binary, configuration, None, ['--clean-run'], 'temporary')
    print('Real application clean-run, MCP, Python and settings isolation passed')


if __name__ == '__main__':
    main()
