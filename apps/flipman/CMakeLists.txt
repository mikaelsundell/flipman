# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

set(app_name "flipman")

# flipman
file (GLOB flipman_sources
    "${CMAKE_SOURCE_DIR}/apps/flipman/*.cpp" 
    "${CMAKE_SOURCE_DIR}/apps/flipman/*.h"
    "${CMAKE_SOURCE_DIR}/apps/flipman/*.ui"
)

file (GLOB flipman_resources
    "${CMAKE_SOURCE_DIR}/apps/flipman/resources/*.frag" 
    "${CMAKE_SOURCE_DIR}/apps/flipman/resources/*.vert"
    "${CMAKE_SOURCE_DIR}/apps/flipman/resources/*.qsb"
)

set (flipman_bundle_resources
    "${CMAKE_SOURCE_DIR}/apps/flipman/resources/MacOSXBundle.plist.in"
)

source_group ("Resource Files" FILES ${flipman_resources})
source_group ("Bundle Files" FILES ${flipman_bundle_resources})

# app program
set (flipman_name ${app_name})
set (flipman_copyright "Copyright 2024-present Contributors to the ${app_name} project")
set (flipman_identifier "com.github.mikaelsundell.jobman")
set (flipman_long_version "0.0.1")
set (flipman_short_version "0.0")
set (flipman_url "https://github.com/mikaelsundell/jobman")

add_definitions (-DFLIPMAN_NAME="${flipman_name}")
add_definitions (-DFLIPMAN_COPYRIGHT="${flipman_copyright}")
add_definitions (-DFLIPMAN_IDENTIFIER="${flipman_identifier}")
add_definitions (-DFLIPMAN_VERSION_STRING="${flipman_long_version}")
add_definitions (-DGITHUBURL="${flipman_url}")

if (APPLE)
    set (MACOSX_BUNDLE_GUI_IDENTIFIER ${flipman_identifier})
    set (MACOSX_BUNDLE_EXECUTABLE_NAME ${flipman_name})
    set (MACOSX_BUNDLE_INFO_STRING ${flipman_name})
    set (MACOSX_BUNDLE_BUNDLE_NAME ${flipman_name})
    set (MACOSX_BUNDLE_ICON_FILE AppIcon.icns)
    set (MACOSX_BUNDLE_LONG_VERSION_STRING ${flipman_long_version})
    set (MACOSX_BUNDLE_SHORT_VERSION_STRING ${flipman_short_version})
    set (MACOSX_BUNDLE_BUNDLE_VERSION ${flipman_long_version})
    set (MACOSX_BUNDLE_COPYRIGHT ${flipman_copyright})
    set (MACOSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
    set (CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
    set_source_files_properties (${flipman_resources} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable (${app_name} MACOSX_BUNDLE ${flipman_sources} ${flipman_resources})
    set_target_properties (${app_name} PROPERTIES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${flipman_identifier}
        #XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET}
        MACOSX_BUNDLE_INFO_PLIST ${flipman_bundle_resources}
    )
    target_link_libraries (${app_name} 
        ${flipman}Sdk
        Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::Widgets 
        "-framework CoreFoundation"
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework AppKit"
    )
else ()
    message (WARNING "${app_name} is a Mac program, will not be built.")
endif ()
