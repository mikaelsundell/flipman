# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Mikael Sundell

find_package(Qt6 COMPONENTS Network REQUIRED)

file(GLOB_RECURSE flipman_component_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/app/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/commands/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/views/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/views/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/python/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/python/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/mcp/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/mcp/*.h")
add_library(flipman_components STATIC ${flipman_component_sources})
target_link_libraries(flipman_components PUBLIC flipmansdk Qt6::Widgets PRIVATE Qt6::Network)
target_include_directories(flipman_components PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(flipman_components PUBLIC QT_NO_KEYWORDS)
target_compile_features(flipman_components PUBLIC cxx_std_17)
set_target_properties(flipman_components PROPERTIES AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/app;${CMAKE_CURRENT_SOURCE_DIR}/views;${CMAKE_CURRENT_SOURCE_DIR}/python")
if(FLIPMAN_APP_PYTHON)
    find_package(Python 3.9 COMPONENTS Interpreter Development.Module Development.Embed REQUIRED)
    execute_process(COMMAND "${Python_EXECUTABLE}" -c "import sys; print(sys.base_prefix)"
        OUTPUT_VARIABLE flipman_python_home OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/python/session.py" flipman_python_bootstrap)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/python/session.py")
    target_link_libraries(flipman_components PRIVATE Python::Python)
    target_compile_definitions(flipman_components PRIVATE FLIPMAN_APP_PYTHON=1 "FLIPMAN_PYTHON_MODULE_DIR=\"$<TARGET_FILE_DIR:flipman_python>\"")
    add_dependencies(flipman_components flipman_python)
else()
    target_compile_definitions(flipman_components PRIVATE FLIPMAN_APP_PYTHON=0)
endif()
configure_file(appconfig.h.in appconfig.h @ONLY)
add_executable(flipman MACOSX_BUNDLE main.cpp)
target_link_libraries(flipman PRIVATE flipman_components)
set_target_properties(flipman PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME Flipman
    MACOSX_BUNDLE_GUI_IDENTIFIER com.mikaelsundell.flipman
    MACOSX_BUNDLE_BUNDLE_VERSION "${project_long_version}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${project_short_version}")
if(APPLE)
    set_target_properties(flipman PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.mikaelsundell.flipman)
endif()

if(BUILD_TESTING)
    add_executable(flipman_application_tests "${PROJECT_SOURCE_DIR}/CTest/application.cpp")
    target_link_libraries(flipman_application_tests PRIVATE flipman_components Qt6::Network)
    add_dependencies(flipman_tests flipman flipman_application_tests)
    foreach(case IN ITEMS session commands viewer services view_lifetime)
        add_test(NAME flipman.app.${case} COMMAND ${CMAKE_COMMAND} -E env
            "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
            $<TARGET_FILE:flipman_application_tests> ${case})
        set_tests_properties(flipman.app.${case} PROPERTIES LABELS "application;native;integration" TIMEOUT 60 RUN_SERIAL TRUE)
    endforeach()
    set_tests_properties(flipman.app.viewer flipman.app.view_lifetime PROPERTIES LABELS "application;native;integration;gpu")
    if(FLIPMAN_APP_PYTHON)
        add_test(NAME flipman.app.clean_run COMMAND ${CMAKE_COMMAND} -E env
            "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
            ${Python_EXECUTABLE} "${PROJECT_SOURCE_DIR}/CTest/application_launch.py" $<TARGET_FILE:flipman> "$<CONFIG>")
        set_tests_properties(flipman.app.clean_run PROPERTIES LABELS "application;mcp;settings;integration;gpu" TIMEOUT 120 RUN_SERIAL TRUE)
        add_test(NAME flipman.app.mcp COMMAND ${CMAKE_COMMAND} -E env
            "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
            $<TARGET_FILE:flipman_application_tests> mcp)
        set_tests_properties(flipman.app.mcp PROPERTIES LABELS "application;mcp;integration" TIMEOUT 60 RUN_SERIAL TRUE)
        add_test(NAME flipman.app.python COMMAND ${CMAKE_COMMAND} -E env
            "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
            $<TARGET_FILE:flipman_application_tests> python)
        set_tests_properties(flipman.app.python PROPERTIES LABELS "application;python;integration" TIMEOUT 60 RUN_SERIAL TRUE)
    endif()
endif()
