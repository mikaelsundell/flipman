# SPDX-License-Identifier: BSD-3-Clause
# Adapted from brawtool's FindBlackmagicRaw.cmake.
# The SDK dispatch loads the framework at runtime; do not link its install path.
set(BlackmagicRaw_ROOT "" CACHE PATH "Blackmagic RAW SDK root (or its Mac directory)")
find_path(BlackmagicRaw_INCLUDE_DIR NAMES BlackmagicRawAPI.h
    HINTS "${BlackmagicRaw_ROOT}" "$ENV{BLACKMAGIC_RAW_ROOT}"
    PATH_SUFFIXES Mac/Include Include)
find_file(BlackmagicRaw_DISPATCH_SOURCE NAMES BlackmagicRawAPIDispatch.cpp
    HINTS "${BlackmagicRaw_INCLUDE_DIR}" "${BlackmagicRaw_ROOT}" "$ENV{BLACKMAGIC_RAW_ROOT}"
    PATH_SUFFIXES Mac/Include Include)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BlackmagicRaw REQUIRED_VARS BlackmagicRaw_INCLUDE_DIR BlackmagicRaw_DISPATCH_SOURCE)
if(BlackmagicRaw_FOUND AND NOT TARGET BlackmagicRaw::Dispatch)
    add_library(flipman_braw_dispatch STATIC "${BlackmagicRaw_DISPATCH_SOURCE}")
    add_library(BlackmagicRaw::Dispatch ALIAS flipman_braw_dispatch)
    target_include_directories(flipman_braw_dispatch SYSTEM PUBLIC "${BlackmagicRaw_INCLUDE_DIR}")
    target_link_libraries(flipman_braw_dispatch PUBLIC "-framework CoreFoundation")
    set_target_properties(flipman_braw_dispatch PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
endif()
mark_as_advanced(BlackmagicRaw_INCLUDE_DIR BlackmagicRaw_DISPATCH_SOURCE)
