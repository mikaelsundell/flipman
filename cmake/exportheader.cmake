# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

function(generate_export_header TARGET_NAME)
    include(GenerateExportHeader)
    if(${ARGC} GREATER 1)
        set(FILENAME "${ARGV1}")
    else()
        set(FILENAME "${TARGET_NAME}export.h")
    endif()
    string(TOUPPER "${TARGET_NAME}" TARGET_UPPER)
    set(EXPORT_MACRO "${TARGET_UPPER}_EXPORT")
    
    generate_export_header(${TARGET_NAME}
        BASE_NAME ${TARGET_NAME}
        EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}"
        EXPORT_MACRO_NAME ${EXPORT_MACRO}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        OBJCXX_VISIBILITY_PRESET hidden
    )

    target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "Generated export header for ${TARGET_NAME}: ${EXPORT_MACRO} (${FILENAME})")
endfunction()