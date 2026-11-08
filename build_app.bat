@echo off
REM SPDX-License-Identifier: BSD-3-Clause
REM Adapted from Mikael Sundell's Stageviz build_app.bat.
setlocal EnableExtensions
set "app_dir=%~dp0"
set "mode=%~1"
set "configure_only=0"
set "fresh="
set "demos=ON"
set "python=OFF"
set "braw=OFF"
set "cmake_generator=Visual Studio 17 2022"
if /I "%mode%"=="--help" goto help
if /I "%mode%"=="debug" (set "mode=debug") else if /I "%mode%"=="release" (set "mode=release") else if /I "%mode%"=="all" (set "mode=all") else goto help
shift
:parse
if "%~1"=="" goto run
set "arg=%~1"
if /I "%arg%"=="--fresh" (set "fresh=--fresh") else if /I "%arg%"=="--configure-only" (set "configure_only=1") else if /I "%arg%"=="--sdk-only" (set "demos=OFF") else if /I "%arg%"=="--braw" (set "braw=ON") else if /I "%arg%"=="--python" (set "python=ON") else if /I "%arg:~0,18%"=="--cmake_generator=" (set "cmake_generator=%arg:~18%") else (
    echo Unsupported argument: %arg%
    exit /b 2
)
shift
goto parse
:run
where cmake >nul 2>&1
if errorlevel 1 (
    echo CMake 3.27 or later must be in PATH.
    exit /b 1
)
if "%mode%"=="all" (
    if not defined THIRDPARTY_DEBUG_DIR goto prefixes
    if not defined THIRDPARTY_RELEASE_DIR goto prefixes
)
echo NOTE: Flipman's SDK still requires a Windows platform port.
echo This entry point will configure/build once the macOS-only SDK dependencies are ported.
if "%mode%"=="all" (
    call :build_one debug Debug
    if errorlevel 1 exit /b 1
    call :build_one release Release
    if errorlevel 1 exit /b 1
    exit /b 0
)
if "%mode%"=="debug" (call :build_one debug Debug) else call :build_one release Release
exit /b %errorlevel%
:prefixes
echo All requires THIRDPARTY_DEBUG_DIR and THIRDPARTY_RELEASE_DIR.
exit /b 2
:build_one
set "build_dir=%app_dir%build.%~1"
set "prefix=%THIRDPARTY_DIR%"
if "%~1"=="debug" if defined THIRDPARTY_DEBUG_DIR set "prefix=%THIRDPARTY_DEBUG_DIR%"
if "%~1"=="release" if defined THIRDPARTY_RELEASE_DIR set "prefix=%THIRDPARTY_RELEASE_DIR%"
if not defined prefix (
    echo Set THIRDPARTY_DIR or the configuration-specific dependency prefix.
    exit /b 2
)
if not exist "%prefix%\" (
    echo Missing dependency prefix: %prefix%
    exit /b 2
)
cmake %fresh% -S "%app_dir%." -B "%build_dir%" -G "%cmake_generator%" -DCMAKE_PREFIX_PATH="%prefix%" -DCMAKE_CONFIGURATION_TYPES=%~2 -DFLIPMAN_BUILD_DEMOS=%demos% -DFLIPMAN_BUILD_PYTHON=%python% -DFLIPMAN_BUILD_BRAW=%braw% -DBlackmagicRaw_ROOT="%BLACKMAGIC_RAW_ROOT%" -DDeckLinkSDK_ROOT="%DeckLinkSDK_ROOT%"
if errorlevel 1 exit /b 1
if "%configure_only%"=="1" exit /b 0
cmake --build "%build_dir%" --config %~2 --parallel
exit /b %errorlevel%
:help
echo usage: build_app.bat debug^|release^|all [--configure-only] [--fresh] [--sdk-only] [--python] [--braw]
echo        [--cmake_generator=GENERATOR]
echo Windows SDK support is pending; CMake configuration cannot succeed until the SDK is ported.
echo Windows deployment is not enabled.
exit /b 0
