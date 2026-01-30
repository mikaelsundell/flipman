# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

cmake_minimum_required (VERSION 3.27)

# project configuration
set (project_name "flipman")
project (${project_name})

set (project_copyright "Copyright 2025-present Contributors to the ${project_name} project")
set (project_identifier "com.github.mikaelsundell.${project_name}")
set (project_long_version "1.0.0")
set (project_short_version "1.0")
set (project_url "https://github.com/mikaelsundell/flipman")

# cmake modules
list (APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# dependencies
set (QT_NO_PRIVATE_MODULE_WARNING ON)
set (qt6_modules Core Concurrent Gui GuiPrivate Widgets ShaderTools)
find_package (Qt6 COMPONENTS ${qt6_modules} CONFIG REQUIRED)

# build settings
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# subdirectories
add_subdirectory (sources/flipman)
add_subdirectory (sources/flipmansdk)

# tests
add_subdirectory (sources/tests/testplay)
add_subdirectory (sources/tests/testsuite)
add_subdirectory (sources/tests/testtimeline)
add_subdirectory (sources/tests/testview)
add_subdirectory (sources/tests/testwidgets)