# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

set(test_name "teststyle")
set(CMAKE_AUTOMOC ON)

file (GLOB test_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(${test_name} ${test_sources})

target_link_libraries(${test_name}
    PRIVATE
        ${project_name}sdk
        Qt6::Core 
        Qt6::Gui 
        Qt6::Widgets
)

target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)