# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

cmake_minimum_required (VERSION 3.27)
set (project_name "flipman")
project (${project_name})

set (qt6_modules Core Concurrent Gui GuiPrivate Widgets ShaderTools)
find_package (Qt6 COMPONENTS ${qt6_modules} CONFIG REQUIRED)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)

# projects
add_subdirectory (sdk)
add_subdirectory (apps/flipman)
add_subdirectory (tests/testgamut)
add_subdirectory (tests/testplay)
add_subdirectory (tests/testsuite)
add_subdirectory (tests/timeline)
