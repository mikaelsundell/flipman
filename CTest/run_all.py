# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 - present Mikael Sundell

"""Incrementally build Flipman tests and run the registered CTest suite."""

import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time
import uuid
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]


def configurations(value):
    return ('Debug', 'Release') if value.lower() == 'all' else (value.capitalize(),)


def cache_value(directory, key):
    cache = directory / 'CMakeCache.txt'
    if cache.is_file():
        for line in cache.read_text(encoding='utf-8', errors='replace').splitlines():
            if line.startswith(key + ':') and '=' in line:
                return line.split('=', 1)[1]
    return None


def tools(directory, override=None):
    candidates = [override] if override else [cache_value(directory, 'CMAKE_COMMAND'), shutil.which('cmake')]
    if not override and sys.platform == 'darwin':
        candidates.append('/Applications/CMake.app/Contents/bin/cmake')
    for candidate in candidates:
        if not candidate:
            continue
        resolved = shutil.which(candidate) or candidate
        if Path(resolved).is_file():
            cmake = Path(resolved).resolve()
            sibling = cmake.with_name('ctest.exe' if sys.platform == 'win32' else 'ctest')
            ctest = str(sibling) if sibling.is_file() else shutil.which('ctest')
            if ctest:
                return str(cmake), str(Path(ctest).resolve())
    raise RuntimeError('CMake and CTest are required. Put them on PATH or pass --cmake PATH.')


def runtime_environment(directory, configuration, platform=None):
    platform = platform or sys.platform
    environment = dict(os.environ)
    environment['PYTHONUNBUFFERED'] = '1'
    if platform == 'darwin':
        if configuration == 'Debug':
            environment['DYLD_IMAGE_SUFFIX'] = '_debug'
        else:
            environment.pop('DYLD_IMAGE_SUFFIX', None)
    if platform == 'win32':
        manifest = directory / ('test-runtime-%s.txt' % configuration)
        if not manifest.is_file():
            raise RuntimeError('Missing %s. Reconfigure before using --no-build.' % manifest)
        paths = [line for line in manifest.read_text(encoding='utf-8').splitlines() if line]
        environment['PATH'] = ';'.join(paths + [environment.get('PATH', '')])
    return environment


def run_logged(command, log, environment, echo=True):
    """Run argument vectors without a shell; preserve combined output and exit status."""
    command = [str(arg) for arg in command]
    print('\nRUN', subprocess.list2cmdline(command), flush=True)
    print('LOG', log, flush=True)
    started = time.monotonic()
    with log.open('w', encoding='utf-8') as stream:
        process = subprocess.Popen(command, cwd=ROOT, env=environment,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                   creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if sys.platform == 'win32' else 0,
                                   start_new_session=sys.platform != 'win32',
                                   text=True, encoding='utf-8', errors='replace')
        try:
            for line in process.stdout:
                stream.write(line)
                stream.flush()
                if echo:
                    print(line, end='', flush=True)
            code = process.wait()
        except BaseException:
            if process.poll() is None:
                try:
                    if sys.platform == 'win32':
                        process.send_signal(signal.CTRL_BREAK_EVENT)
                    else:
                        os.killpg(process.pid, signal.SIGINT)
                except OSError:
                    pass
                try:
                    process.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    if sys.platform == 'win32':
                        process.kill()
                    else:
                        os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
            raise
        finally:
            process.stdout.close()
    return {'command': command, 'returncode': code, 'seconds': time.monotonic() - started, 'log': str(log)}


def ctest_command(ctest, directory, configuration, args):
    command = [ctest, '--test-dir', str(directory), '-C', configuration]
    for option, value in (('-L', args.label), ('-LE', args.exclude_label), ('-R', args.test)):
        if value:
            command.extend((option, value))
    return command


def read_manifest(path):
    manifest = json.loads(path.read_text(encoding='utf-8'))
    tests = manifest.get('tests', [])
    if not tests:
        raise RuntimeError('No registered tests match the selection.')
    return tests


def disabled(test):
    return any(prop['name'] == 'DISABLED' and prop['value'] for prop in test.get('properties', []))


def read_results(path, tests):
    """Distinguish disabled tests, genuine skips and failures instead of calling all green."""
    counts = {key: 0 for key in ('passed', 'failed', 'skipped', 'disabled', 'not_run')}
    rows = []
    disabled_names = {test['name'] for test in tests if disabled(test)}
    selected_names = {test['name'] for test in tests}
    seen = set()
    for case in ET.parse(path).getroot().iter('testcase'):
        name = case.attrib['name']
        seen.add(name)
        if name in disabled_names:
            status = 'disabled'
        elif case.find('failure') is not None or case.find('error') is not None:
            status = 'failed'
        elif case.find('skipped') is not None:
            status = 'skipped'
        elif case.attrib.get('status') == 'notrun':
            status = 'not_run'
        else:
            status = 'passed'
        counts[status] += 1
        rows.append({'name': name, 'status': status, 'seconds': float(case.attrib.get('time', 0))})
    missing = selected_names - seen
    for name in sorted(missing):
        status = 'disabled' if name in disabled_names else 'not_run'
        counts[status] += 1
        rows.append({'name': name, 'status': status, 'seconds': 0})
    return {'selected': len(tests), **counts, 'cases': rows}


def build_directory(args, configuration):
    return Path(args.build_dir).resolve() if args.build_dir else ROOT / ('build.' + configuration.lower())


def validate_build(directory, configuration):
    if not (directory / 'CMakeCache.txt').is_file():
        raise RuntimeError('No configured build at %s. Run build_app for %s first.' % (directory, configuration))
    source = cache_value(directory, 'CMAKE_HOME_DIRECTORY')
    if source and Path(source).resolve() != ROOT.resolve():
        raise RuntimeError('The build belongs to another source checkout: %s' % source)
    available = cache_value(directory, 'CMAKE_CONFIGURATION_TYPES')
    if available and configuration not in available.split(';'):
        raise RuntimeError('%s is not configured for %s.' % (directory, configuration))
    single = cache_value(directory, 'CMAKE_BUILD_TYPE')
    if not available and single and single.lower() != configuration.lower():
        raise RuntimeError('%s is a %s build, not %s.' % (directory, single, configuration))


def run_configuration(args, configuration, output):
    directory = build_directory(args, configuration)
    row = {'configuration': configuration, 'build_dir': str(directory), 'passed': False, 'steps': []}
    output.mkdir()
    try:
        validate_build(directory, configuration)
        cmake, ctest = tools(directory, args.cmake)
        # Configure/build tools do not need test DLL directories, which are
        # generated by configuration itself on Windows.
        environment = dict(os.environ)
        environment['PYTHONUNBUFFERED'] = '1'
        if sys.platform == 'darwin':
            if configuration == 'Debug':
                environment['DYLD_IMAGE_SUFFIX'] = '_debug'
            else:
                environment.pop('DYLD_IMAGE_SUFFIX', None)
        if not args.no_build:
            step = run_logged([cmake, '-S', ROOT, '-B', directory, '-DBUILD_TESTING=ON'],
                              output / 'configure.log', environment)
            row['steps'].append(dict(name='configure', **step))
            if step['returncode']:
                raise RuntimeError('Configure failed; tests were not run against stale executables.')
            step = run_logged([cmake, '--build', directory, '--config', configuration,
                               '--target', 'flipman_tests', '--parallel', str(args.jobs)],
                              output / 'build.log', environment)
            row['steps'].append(dict(name='build', **step))
            if step['returncode']:
                raise RuntimeError('Build failed; tests were not run against stale executables.')
        if cache_value(directory, 'BUILD_TESTING') in ('OFF', 'FALSE', '0'):
            raise RuntimeError('BUILD_TESTING is disabled; rerun without --no-build.')
        row['features'] = {key: cache_value(directory, key) for key in
                           ('FLIPMAN_BUILD_PYTHON', 'FLIPMAN_BUILD_BRAW', 'FLIPMAN_REQUIRE_TEST_DATA')}
        print('FEATURES', row['features'], flush=True)
        environment = runtime_environment(directory, configuration)
        command = ctest_command(ctest, directory, configuration, args)
        manifest = output / 'tests.json'
        step = run_logged(command + ['--show-only=json-v1'], manifest, environment, echo=False)
        row['steps'].append(dict(name='discover', **step))
        if step['returncode']:
            raise RuntimeError('CTest discovery failed.')
        tests = read_manifest(manifest)
        if all(disabled(test) for test in tests):
            raise RuntimeError('The selection contains only disabled tests.')
        junit = output / 'ctest.xml'
        step = run_logged(command + ['--output-on-failure', '--no-tests=error', '--parallel', '1',
                                     '--output-junit', str(junit)], output / 'ctest.log', environment)
        row['steps'].append(dict(name='ctest', **step))
        row['tests'] = read_results(junit, tests)
        counts = {key: value for key, value in row['tests'].items() if key != 'cases'}
        print('RESULTS', configuration, counts, flush=True)
        if row['tests']['failed'] or row['tests']['not_run']:
            raise RuntimeError('Some enabled tests failed or did not run.')
        if args.fail_on_skip and row['tests']['skipped']:
            raise RuntimeError('Enabled tests were skipped (--fail-on-skip).')
        row['passed'] = step['returncode'] == 0
    except Exception as error:
        row['error'] = str(error)
        print('FAIL', configuration, error, flush=True)
    return row


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('configuration', nargs='?', default='debug', type=str.lower,
                        choices=('debug', 'release', 'all'))
    parser.add_argument('--no-build', action='store_true', help='Use existing binaries without rebuilding')
    parser.add_argument('--output', help='New directory for logs, JSON and JUnit reports')
    parser.add_argument('--build-dir', help='Configured build directory (single configuration only)')
    parser.add_argument('--cmake', help='CMake executable when absent from PATH/build cache')
    parser.add_argument('--jobs', type=int, default=min(os.cpu_count() or 2, 6),
                        help='Parallel build jobs; tests remain serial')
    parser.add_argument('--list', action='store_true', help='List registered tests without building/running')
    parser.add_argument('--label', help='CTest label regex (-L)')
    parser.add_argument('--exclude-label', help='CTest label exclusion regex (-LE)')
    parser.add_argument('--test', help='CTest test-name regex (-R)')
    parser.add_argument('--fail-on-skip', action='store_true', help='Fail if an enabled test is skipped')
    args = parser.parse_args(argv)
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    if args.configuration == 'all' and args.build_dir:
        parser.error('--build-dir requires a single configuration')
    if args.list:
        try:
            for configuration in configurations(args.configuration):
                directory = build_directory(args, configuration)
                validate_build(directory, configuration)
                _, ctest = tools(directory, args.cmake)
                result = subprocess.run(ctest_command(ctest, directory, configuration, args) + ['--show-only=json-v1'],
                                        cwd=ROOT, env=runtime_environment(directory, configuration),
                                        capture_output=True, text=True, encoding='utf-8', errors='replace')
                if result.returncode:
                    raise RuntimeError(result.stderr or result.stdout)
                tests = json.loads(result.stdout).get('tests', [])
                if not tests:
                    raise RuntimeError('No registered tests match the selection.')
                print('%s: %d registered tests' % (configuration, len(tests)))
                for test in tests:
                    labels = next((prop['value'] for prop in test.get('properties', []) if prop['name'] == 'LABELS'), [])
                    print('  %s%s [%s]' % (test['name'], ' (disabled)' if disabled(test) else '', ';'.join(labels)))
            return 0
        except Exception as error:
            print('FAIL', error, file=sys.stderr)
            return 1
    stamp = datetime.now().strftime('%Y%m%d-%H%M%S') + '-' + uuid.uuid4().hex[:8]
    output = Path(args.output).resolve() if args.output else ROOT / 'build.test-results' / stamp
    try:
        output.mkdir(parents=True)
    except FileExistsError:
        parser.error('--output must be a new directory: %s' % output)
    summary = {'passed': False, 'status': 'running', 'configurations': []}
    summary_file = output / 'summary.json'
    print('Flipman tests. Results:', output, flush=True)
    try:
        summary_file.write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
        for configuration in configurations(args.configuration):
            summary['configurations'].append(run_configuration(args, configuration, output / configuration.lower()))
            summary_file.write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
    except KeyboardInterrupt:
        summary['status'] = 'interrupted'
        summary_file.write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
        print('\nInterrupted:', summary_file, flush=True)
        return 130
    summary['passed'] = all(row['passed'] for row in summary['configurations'])
    skipped = any(row.get('tests', {}).get('skipped', 0) for row in summary['configurations'])
    summary['status'] = ('passed_with_skips' if skipped else 'passed') if summary['passed'] else 'failed'
    summary_file.write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
    print('\n%s: %s' % (summary['status'].upper(), summary_file), flush=True)
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
