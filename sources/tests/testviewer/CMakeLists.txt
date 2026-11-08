# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

set(test_name "testviewer")

file (GLOB test_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(${test_name} ${test_sources})

target_link_libraries(${test_name} 
    flipmansdk
)
target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR})

if(BUILD_TESTING)
    add_executable(flipman_viewerdrop_tests ${PROJECT_SOURCE_DIR}/CTest/viewerdrop.cpp window.cpp window.h)
    target_include_directories(flipman_viewerdrop_tests PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(flipman_viewerdrop_tests PRIVATE flipmansdk)
    add_test(NAME flipman.viewer.drop
        COMMAND ${CMAKE_COMMAND} -E env
            "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
            QT_QPA_PLATFORM=offscreen
            $<TARGET_FILE:flipman_viewerdrop_tests>)
    set_tests_properties(flipman.viewer.drop PROPERTIES LABELS "viewer;native;integration" TIMEOUT 60)
    add_dependencies(flipman_tests flipman_viewerdrop_tests)
endif()
