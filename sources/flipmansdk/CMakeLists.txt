# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

set(sdk_name "${project_name}sdk")

include(exportheader)

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

# plugins
file(GLOB sdk_plugins_base_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/plugins/*.h")
file(GLOB sdk_plugins_base_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*.cpp" 
)
source_group("Header Files\\plugins" FILES ${sdk_plugins_base_headers})
source_group("Source Files\\plugins" FILES ${sdk_plugins_base_sources})

set(plugin_subdirs "braw" "fx" "oiio" "qt" "quicktime")
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

# render
file(GLOB sdk_render_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/render/*.h")
file(GLOB sdk_render_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp" 
)
source_group("Header Files\\render" FILES ${sdk_render_headers})
source_group("Source Files\\render" FILES ${sdk_render_sources})

# widgets
file(GLOB sdk_widgets_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/widgets/*.h")
file(GLOB sdk_widgets_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.cpp" 
)
source_group("Header Files\\widgets" FILES ${sdk_widgets_headers})
source_group("Source Files\\widgets" FILES ${sdk_widgets_sources})

# resources
file(GLOB sdk_root_headers
     "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/*.h")

file(GLOB sdk_resources
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qss")

file(GLOB sdk_glsl_resources
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/glsl/*.glsl"
)

source_group("Resources" FILES ${sdk_resources})
source_group("Resources\\glsl" FILES ${sdk_glsl_resources})

# target definition
add_library(${sdk_name} SHARED
    ${sdk_root_headers}
    ${sdk_av_headers}
    ${sdk_av_sources}
    ${sdk_core_headers}
    ${sdk_core_sources}
    ${sdk_plugins_all_headers}
    ${sdk_plugins_all_sources}
    ${sdk_render_headers}
    ${sdk_render_sources}
    ${sdk_widgets_headers}
    ${sdk_widgets_sources}
)

qt_add_resources(${sdk_name} "resources"
    PREFIX "/flipmansdk"
    BASE "resources"
    FILES
        ${sdk_resources}
        ${sdk_glsl_resources}
)

target_sources(${sdk_name}
    PRIVATE
        ${sdk_resources}
        ${sdk_glsl_resources}
)

target_link_libraries(${sdk_name}
    PRIVATE
        ${sdk_render_shaders}
)

target_compile_definitions(${sdk_name} PRIVATE
    -DPROJECT_NAME="${project_name}"
    -DPROJECT_VERSION="${project_long_version}"
    -DPROJECT_COPYRIGHT="${project_copyright}"
    -DPROJECT_IDENTIFIER="${project_identifier}"
    -DGITHUB_URL="${project_url}"
)

target_include_directories(${sdk_name} 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

generate_export_header(${sdk_name} 
    BASE_NAME FLIPMANSDK
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/flipmanexport.h"
)

target_link_libraries(${sdk_name}
    PUBLIC
        Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::ShaderTools Qt6::Widgets
    PRIVATE
        Qt6::GuiPrivate
        Qt6::ShaderToolsPrivate
        OpenImageIO::OpenImageIO
        OpenImageIO::OpenImageIO_Util
        "-framework CoreFoundation"
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework AppKit"
)