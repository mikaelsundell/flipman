# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

set(sdk_name "${project_name}sdk")

include(exportheader)
include(shadercompile)

# av
file(GLOB sdk_av_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/av/*.h")
file(GLOB sdk_av_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/av/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/av/*.mm"
)
source_group("Header Files\\av" FILES ${sdk_av_headers})
source_group("Source Files\\av" FILES ${sdk_av_sources})

# core
file(GLOB sdk_core_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/core/*.h")
file(GLOB sdk_core_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.mm"
)
source_group("Header Files\\core" FILES ${sdk_core_headers})
source_group("Source Files\\core" FILES ${sdk_core_sources})

# log
file(GLOB sdk_log_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/log/*.h")
file(GLOB sdk_log_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/log/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/log/*.mm"
)
source_group("Header Files\\log" FILES ${sdk_log_headers})
source_group("Source Files\\log" FILES ${sdk_log_sources})

# plugins
file(GLOB sdk_plugins_base_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/plugins/*.h")
file(GLOB sdk_plugins_base_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.mm"
)
source_group("Header Files\\plugins" FILES ${sdk_plugins_base_headers})
source_group("Source Files\\plugins" FILES ${sdk_plugins_base_sources})

set(plugin_subdirs "braw" "oiio" "qt" "quicktime")
set(sdk_plugins_all_headers ${sdk_plugins_base_headers})
set(sdk_plugins_all_sources ${sdk_plugins_base_sources})

foreach(subdir ${plugin_subdirs})
    file(GLOB subdir_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/plugins/${subdir}/*.h")
    file(GLOB subdir_sources 
        "${CMAKE_CURRENT_SOURCE_DIR}/plugins/${subdir}/*.cpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/plugins/${subdir}/*.mm"
    )
    
    source_group("Header Files\\plugins\\${subdir}" FILES ${subdir_headers})
    source_group("Source Files\\plugins\\${subdir}" FILES ${subdir_sources})
    
    list(APPEND sdk_plugins_all_headers ${subdir_headers})
    list(APPEND sdk_plugins_all_sources ${subdir_sources})
endforeach()

# widgets
file(GLOB sdk_widgets_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/widgets/*.h")
file(GLOB sdk_widgets_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.mm"
)
source_group("Header Files\\widgets" FILES ${sdk_widgets_headers})
source_group("Source Files\\widgets" FILES ${sdk_widgets_sources})

# shaders
file(GLOB sdk_shaders_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
)
source_group("Source Files\\shaders" FILES ${sdk_shaders_sources})

# Generate QSB shaders
set(sdk_shaders_qrc_in "${CMAKE_SOURCE_DIR}/sources/flipmansdk/shaders/shaders.qrc.in")
generate_qt_shaders(
    ${sdk_name} 
    "${sdk_shaders_sources}" 
    "${sdk_shaders_qrc_in}"
    sdk_qsb_shaders 
    sdk_shaders_qsb_sources
)

# resources & root headers
file(GLOB sdk_root_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/*.h")
file(GLOB sdk_resources "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qss")
set(sdk_qrc_file "${CMAKE_CURRENT_SOURCE_DIR}/resources/flipmansdk.qrc")
qt_add_resources(sdk_resource_sources ${sdk_qrc_file})

source_group("Header Files" FILES ${sdk_root_headers})
source_group("Resources" FILES ${sdk_resources} ${sdk_resource_sources})

# target definition
add_library(${sdk_name} SHARED
    ${sdk_root_headers}
    ${sdk_av_headers}
    ${sdk_av_sources}
    ${sdk_core_headers}
    ${sdk_core_sources}
    ${sdk_log_headers}
    ${sdk_log_sources}
    ${sdk_plugins_all_headers}
    ${sdk_plugins_all_sources}
    ${sdk_widgets_headers}
    ${sdk_widgets_sources}
    ${sdk_shaders_sources}     # original sources for IDE editing
    ${sdk_shaders_qsb_sources} # compiled binary shaders
    ${sdk_resources}
    ${sdk_resource_sources}
)

target_compile_definitions(${sdk_name} PRIVATE
    -DPROJECT_NAME="${project_name}"
    -DPROJECT_VERSION="${project_long_version}"
    -DPROJECT_COPYRIGHT="${project_copyright}"
    -DPROJECT_IDENTIFIER="${project_identifier}"
    -DGITHUB_URL="${project_url}"
)

# usage requirements: PUBLIC ensures that targets linking to the SDK
# automatically get the include/ directory in their search path.
target_include_directories(${sdk_name} 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR} # For private headers or .cpp files finding their own headers
)

generate_export_header(${sdk_name} 
    BASE_NAME FLIPMANSDK
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/flipmanexport.h"
)

target_link_libraries(${sdk_name}
    PUBLIC
        Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::Widgets
    PRIVATE
        Qt6::GuiPrivate
        "-framework CoreFoundation"
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework AppKit"
)

add_dependencies(${sdk_name} ${sdk_name}_shaders)