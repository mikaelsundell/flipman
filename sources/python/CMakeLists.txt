# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Python 3.9 COMPONENTS Interpreter Development.Module REQUIRED)
# Resolve launcher shims such as /usr/bin/python3 to the interpreter they select.
# Keep sys.executable rather than resolving symlinks, to preserve virtualenvs.
execute_process(COMMAND "${Python_EXECUTABLE}" -c "import sys; print(sys.executable)"
    OUTPUT_VARIABLE python_runtime OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE python_runtime_result)
if(NOT python_runtime_result EQUAL 0 OR NOT EXISTS "${python_runtime}")
    message(FATAL_ERROR "Could not resolve the selected Python runtime")
endif()
set(Python_EXECUTABLE "${python_runtime}")
find_package(nanobind CONFIG QUIET HINTS ${CMAKE_PREFIX_PATH} PATH_SUFFIXES nanobind/cmake)
if(NOT nanobind_FOUND)
    execute_process(COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
        OUTPUT_VARIABLE nanobind_DIR OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE nanobind_result)
    if(NOT nanobind_result EQUAL 0)
        message(FATAL_ERROR "Install nanobind for the selected Python or set nanobind_DIR")
    endif()
    find_package(nanobind CONFIG REQUIRED)
endif()
nanobind_add_module(flipman_python NB_STATIC module.cpp av.cpp core.cpp media.cpp render.cpp bindings.h)
set_target_properties(flipman_python PROPERTIES
    OUTPUT_NAME flipman LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/python/$<CONFIG>"
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(flipman_python PRIVATE flipmansdk)
target_compile_definitions(flipman_python PRIVATE QT_NO_KEYWORDS PROJECT_VERSION="${project_long_version}")
# Export the selected interpreter to the sibling CTest directory.
set(Python_EXECUTABLE "${Python_EXECUTABLE}" PARENT_SCOPE)

# Run the selected interpreter directly. macOS's protected /usr/bin/python3
# launcher can strip DYLD settings before it reaches the real interpreter.
file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/python/$<CONFIG>/flipman-python"
    CONTENT "#!/bin/sh\nexec \"${CMAKE_COMMAND}\" -E env \"DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>\" \"PYTHONPATH=$<TARGET_FILE_DIR:flipman_python>\${PYTHONPATH:+:\$PYTHONPATH}\" \"${Python_EXECUTABLE}\" \"\$@\"\n"
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
