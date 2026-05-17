# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

function(configure_export_header TARGET_NAME)
    include(GenerateExportHeader)

    cmake_parse_arguments(ARG "" "BASE_NAME;EXPORT_FILE_NAME" "" ${ARGN})

    if(NOT ARG_BASE_NAME)
        set(ARG_BASE_NAME ${TARGET_NAME})
    endif()

    if(NOT ARG_EXPORT_FILE_NAME)
        set(ARG_EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}export.h")
    endif()

    string(TOUPPER "${ARG_BASE_NAME}" BASE_UPPER)
    set(EXPORT_MACRO "${BASE_UPPER}_EXPORT")

    generate_export_header(${TARGET_NAME}
        BASE_NAME ${ARG_BASE_NAME}
        EXPORT_FILE_NAME "${ARG_EXPORT_FILE_NAME}"
        EXPORT_MACRO_NAME ${EXPORT_MACRO}
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        OBJCXX_VISIBILITY_PRESET hidden
    )

    target_include_directories(${TARGET_NAME} PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    )

    message(STATUS "Generated export header for ${TARGET_NAME}: ${EXPORT_MACRO}")
endfunction()