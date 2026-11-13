# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 - present Mikael Sundell

"""Controller regressions: build failures, selection, runtime and truthful reports."""

from contextlib import redirect_stdout, redirect_stderr
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock


spec = importlib.util.spec_from_file_location('flipman_test_runner', Path(__file__).with_name('run_all.py'))
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class RunnerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='flipman runner ')
        self.root = Path(self.temporary.name).resolve()
        self.patch = mock.patch.object(runner, 'ROOT', self.root)
        self.patch.start()
        for config in ('Debug', 'Release'):
            directory = self.root / ('build.' + config.lower())
            directory.mkdir()
            (directory / 'CMakeCache.txt').write_text(
                'CMAKE_HOME_DIRECTORY:INTERNAL=%s\nCMAKE_CONFIGURATION_TYPES:STRING=%s\n'
                'BUILD_TESTING:BOOL=ON\nFLIPMAN_BUILD_PYTHON:BOOL=ON\n' % (self.root, config), encoding='utf-8')
        self.args = SimpleNamespace(build_dir=None, cmake=None, jobs=2, no_build=False,
                                    label=None, exclude_label=None, test=None, fail_on_skip=False)
        self.commands = []
        self.fail_step = None
        self.manifest = {'tests': [{'name': 'flipman.example', 'properties': []}]}
        self.junit = '<testsuite><testcase name="flipman.example" status="run" time="0.01"/></testsuite>'

    def tearDown(self):
        self.patch.stop()
        self.temporary.cleanup()

    def fake_step(self, command, log, environment, echo=True):
        command = [str(part) for part in command]
        self.commands.append(command)
        phase = 'configure' if '-S' in command else 'build' if '--build' in command else 'ctest'
        if '--show-only=json-v1' in command:
            log.write_text(json.dumps(self.manifest), encoding='utf-8')
            phase = 'discover'
        else:
            log.write_text('test log\n', encoding='utf-8')
        if '--output-junit' in command:
            Path(command[command.index('--output-junit') + 1]).write_text(self.junit, encoding='utf-8')
        return {'returncode': int(self.fail_step == phase), 'log': str(log), 'command': command, 'seconds': 0}

    def run_configuration(self):
        output = self.root / ('results%d' % len(list(self.root.glob('results*'))))
        with mock.patch.object(runner, 'tools', return_value=('cmake', 'ctest')), \
                mock.patch.object(runner, 'run_logged', side_effect=self.fake_step), \
                mock.patch.object(runner, 'runtime_environment', return_value={}), redirect_stdout(io.StringIO()):
            return runner.run_configuration(self.args, 'Debug', output)

    def test_configuration_environment_and_windows_runtime(self):
        self.assertEqual(runner.configurations('ALL'), ('Debug', 'Release'))
        directory = self.root / 'build.debug'
        with mock.patch.dict(os.environ, {'DYLD_IMAGE_SUFFIX': '_debug', 'PATH': 'parent'}, clear=True):
            debug = runner.runtime_environment(directory, 'Debug', platform='darwin')
            release = runner.runtime_environment(directory, 'Release', platform='darwin')
            self.assertEqual(debug['DYLD_IMAGE_SUFFIX'], '_debug')
            self.assertNotIn('DYLD_IMAGE_SUFFIX', release)
            with self.assertRaisesRegex(RuntimeError, 'Reconfigure'):
                runner.runtime_environment(directory, 'Debug', platform='win32')
            (directory / 'test-runtime-Debug.txt').write_text('C:\\debug deps\\bin\nC:\\sdk\\Debug\n')
            windows = runner.runtime_environment(directory, 'Debug', platform='win32')
            self.assertEqual(windows['PATH'], 'C:\\debug deps\\bin;C:\\sdk\\Debug;parent')

    def test_tools_support_spaces_and_override(self):
        directory = self.root / 'tool suite'
        directory.mkdir()
        suffix = '.exe' if sys.platform == 'win32' else ''
        cmake = directory / ('cmake' + suffix)
        ctest = directory / ('ctest' + suffix)
        cmake.touch()
        ctest.touch()
        self.assertEqual(runner.tools(self.root, str(cmake)), (str(cmake), str(ctest)))
        with self.assertRaises(RuntimeError):
            runner.tools(self.root, str(directory / 'missing'))

    def test_build_failures_never_run_stale_tests(self):
        for phase in ('configure', 'build'):
            self.commands.clear()
            self.fail_step = phase
            result = self.run_configuration()
            self.assertFalse(result['passed'])
            self.assertIn('stale executables', result['error'])
            self.assertFalse(any('--show-only=json-v1' in command for command in self.commands))
            self.assertFalse(any('--output-junit' in command for command in self.commands))

    def test_build_target_filters_and_no_build(self):
        self.args.label, self.args.exclude_label, self.args.test = 'cpu', 'data', '^flipman[.]'
        result = self.run_configuration()
        self.assertTrue(result['passed'])
        self.assertEqual(result['tests']['passed'], 1)
        build = next(command for command in self.commands if '--build' in command)
        self.assertIn('flipman_tests', build)
        ctest = self.commands[-1]
        self.assertIn('--no-tests=error', ctest)
        self.assertEqual(ctest[ctest.index('--parallel') + 1], '1')
        for option, value in (('-L', 'cpu'), ('-LE', 'data'), ('-R', '^flipman[.]')):
            self.assertEqual(ctest[ctest.index(option) + 1], value)
        self.args.no_build = True
        self.commands.clear()
        self.assertTrue(self.run_configuration()['passed'])
        self.assertFalse(any('--build' in command or '-S' in command for command in self.commands))

    def test_skips_and_disabled_are_not_passes(self):
        self.manifest['tests'] += [{'name': 'skip', 'properties': []},
                                   {'name': 'timer', 'properties': [{'name': 'DISABLED', 'value': True}]}]
        self.junit = ('<testsuite><testcase name="flipman.example"/>'
                      '<testcase name="skip"><skipped/></testcase>'
                      '<testcase name="timer"><skipped message="Disabled"/></testcase></testsuite>')
        result = self.run_configuration()
        self.assertTrue(result['passed'])
        self.assertEqual([result['tests'][key] for key in ('passed', 'skipped', 'disabled')], [1, 1, 1])
        self.args.fail_on_skip = True
        self.assertFalse(self.run_configuration()['passed'])
        self.manifest['tests'] = [{'name': 'timer', 'properties': [{'name': 'DISABLED', 'value': True}]}]
        self.assertIn('only disabled', self.run_configuration()['error'])

    def test_empty_selection_failed_process_and_missing_results(self):
        self.manifest['tests'] = []
        self.assertFalse(self.run_configuration()['passed'])
        self.manifest['tests'] = [{'name': 'flipman.example', 'properties': []}]
        self.fail_step = 'ctest'
        self.assertFalse(self.run_configuration()['passed'])
        self.fail_step = None
        self.junit = '<testsuite/>'
        self.assertEqual(self.run_configuration()['tests']['not_run'], 1)

    def test_cache_mismatch_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'not configured'):
            runner.validate_build(self.root / 'build.debug', 'Release')
        with self.assertRaisesRegex(RuntimeError, 'No configured build'):
            runner.validate_build(self.root / 'missing', 'Debug')

    def test_all_continues_and_saves_failure_status(self):
        output = self.root / 'output with spaces'
        with mock.patch.object(runner, 'run_configuration', side_effect=[{'passed': False}, {'passed': True}]) as run, \
                redirect_stdout(io.StringIO()):
            self.assertEqual(runner.main(['all', '--output', str(output)]), 1)
        self.assertEqual(run.call_count, 2)
        summary = json.loads((output / 'summary.json').read_text())
        self.assertEqual(summary['status'], 'failed')
        self.assertEqual(len(summary['configurations']), 2)
        with redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
            runner.main(['debug', '--output', str(output)])
        self.assertEqual(error.exception.code, 2)

    def test_process_exit_status_and_logs(self):
        log = self.root / 'child log.txt'
        with redirect_stdout(io.StringIO()):
            result = runner.run_logged([sys.executable, '-c', 'print("child output"); raise SystemExit(7)'],
                                       log, dict(os.environ), echo=False)
        self.assertEqual(result['returncode'], 7)
        self.assertEqual(log.read_text().strip(), 'child output')


if __name__ == '__main__':
    unittest.main()
