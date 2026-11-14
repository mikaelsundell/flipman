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

option(FLIPMAN_BUILD_APP "Build the Flipman application" ON)
option(FLIPMAN_APP_PYTHON "Embed Python in the application" ON)
option(FLIPMAN_BUILD_DEMOS "Build interactive SDK demos" ON)
option(FLIPMAN_BUILD_BRAW "Build the Blackmagic RAW reader (requires the RAW SDK)" OFF)
option(FLIPMAN_BUILD_PYTHON "Build the standalone SDK Python extension" OFF)
if(FLIPMAN_BUILD_APP AND FLIPMAN_APP_PYTHON)
    set(FLIPMAN_BUILD_PYTHON ON CACHE BOOL "Build the standalone SDK Python extension" FORCE)
endif()

# Organize targets in IDEs that support CMake folders (Xcode and Visual Studio).
# Qt's generated helper targets stay together instead of cluttering each group.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Build")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Generated/Qt")
set_property(GLOBAL PROPERTY QT_TARGETS_FOLDER "Generated/Qt")
include(CTest)

get_property(flipman_dashboard_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS flipman_dashboard_targets)
    get_target_property(folder ${target} FOLDER)
    if(folder STREQUAL "CTestDashboardTargets")
        set_property(TARGET ${target} PROPERTY FOLDER "Build/CTest")
    endif()
endforeach()

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
set(CMAKE_FOLDER "Third-party")
add_subdirectory (3rdparty)
set(CMAKE_FOLDER "SDK")
add_subdirectory (sources/flipmansdk)

if(FLIPMAN_BUILD_PYTHON)
    set(CMAKE_FOLDER "Python")
    add_subdirectory(sources/python)
endif()

if(BUILD_TESTING)
    set(CMAKE_FOLDER "Tests")
    add_subdirectory(CTest)
endif()

# Interactive demos are separate from automated SDK regressions.
if(FLIPMAN_BUILD_DEMOS)
    set(CMAKE_FOLDER "Demos")
    add_subdirectory (sources/tests/testdecklink)
    add_subdirectory (sources/tests/testfx)
    #add_subdirectory (sources/tests/testplay)
    #add_subdirectory (sources/tests/testtimeline)
    add_subdirectory (sources/tests/teststyle)
    add_subdirectory (sources/tests/testviewer)
    #add_subdirectory (sources/tests/testwidgets)
endif()

if(FLIPMAN_BUILD_APP)
    set(CMAKE_FOLDER "Application")
    add_subdirectory(sources/flipman)
endif()
# The Xcode generator creates ALL_BUILD after processing this file.
set(CMAKE_FOLDER "Build")

# A few targets are defined outside the directory that represents their role.
foreach(target IN ITEMS flipmansdk_headers flipman_viewerdrop_tests flipman_application_tests)
    if(TARGET ${target})
        set_property(TARGET ${target} PROPERTY FOLDER "Tests")
    endif()
endforeach()
foreach(target IN ITEMS nanobind-static DeckLinkSDK flipman_braw_dispatch)
    if(TARGET ${target})
        set_property(TARGET ${target} PROPERTY FOLDER "Third-party")
    endif()
endforeach()
if(TARGET flipman_imageconverter_benchmark)
    set_property(TARGET flipman_imageconverter_benchmark PROPERTY FOLDER "Benchmarks")
endif()

# macOS Debug must select the matching Qt images before process startup.
function(flipman_debug_schemes directory)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(kind ${target} TYPE)
        if(kind STREQUAL "EXECUTABLE")
            set_target_properties(${target} PROPERTIES XCODE_GENERATE_SCHEME ON
                XCODE_SCHEME_ENVIRONMENT "DYLD_IMAGE_SUFFIX=_debug")
        endif()
    endforeach()
    get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        flipman_debug_schemes("${child}")
    endforeach()
endfunction()
if(APPLE AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES STREQUAL "Debug"))
    flipman_debug_schemes("${PROJECT_SOURCE_DIR}")
endif()
