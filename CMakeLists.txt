# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

cmake_minimum_required (VERSION 3.27)

# Keep direct CMake configuration consistent with build_app.sh. Set
# this before project() so compiler checks and generated Xcode targets agree.
# A newer SDK does not mean the application requires that newer OS at runtime.
if(CMAKE_HOST_APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    if(NOT "$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
        set(flipman_deployment_target "$ENV{MACOSX_DEPLOYMENT_TARGET}")
    else()
        execute_process(COMMAND /usr/bin/sw_vers -productVersion
            OUTPUT_VARIABLE flipman_host_macos OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY)
        string(REGEX MATCH "^[0-9]+" flipman_deployment_target "${flipman_host_macos}")
    endif()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${flipman_deployment_target}" CACHE STRING
        "Minimum macOS version supported by the build" FORCE)
endif()

# project configuration
set (project_name "flipman")
project (${project_name})

if(APPLE)
    enable_language(OBJCXX)
endif()

option(FLIPMAN_BUILD_DEMOS "Build interactive SDK demos" ON)
option(FLIPMAN_BUILD_BRAW "Build the Blackmagic RAW reader (requires the RAW SDK)" OFF)
option(FLIPMAN_BUILD_PYTHON "Build the standalone SDK Python extension" OFF)
include(CTest)

set (project_copyright "Copyright 2025-present Contributors to the ${project_name} project")
set (project_identifier "com.github.mikaelsundell.${project_name}")
set (project_long_version "1.0.0")
set (project_short_version "1.0")
set (project_url "https://github.com/mikaelsundell/flipman")

# cmake modules
list (APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# dependencies
set (QT_NO_PRIVATE_MODULE_WARNING ON)
set (qt6_modules Core Concurrent Gui GuiPrivate Widgets ShaderTools ShaderToolsPrivate)
find_package (Qt6 COMPONENTS ${qt6_modules} CONFIG REQUIRED)
find_package (OpenImageIO CONFIG REQUIRED)
find_package(OpenColorIO 2.5 CONFIG REQUIRED)

# build settings
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# subdirectories
#add_subdirectory (sources/flipman)
add_subdirectory (3rdparty)
add_subdirectory (sources/flipmansdk)

if(FLIPMAN_BUILD_PYTHON)
    add_subdirectory(sources/python)
endif()

if(BUILD_TESTING)
    add_subdirectory(CTest)
endif()

# Interactive demos are separate from automated SDK regressions.
if(FLIPMAN_BUILD_DEMOS)
    add_subdirectory (sources/tests/testdecklink)
    add_subdirectory (sources/tests/testfx)
    #add_subdirectory (sources/tests/testplay)
    #add_subdirectory (sources/tests/testtimeline)
    add_subdirectory (sources/tests/teststyle)
    add_subdirectory (sources/tests/testviewer)
    #add_subdirectory (sources/tests/testwidgets)
endif()
