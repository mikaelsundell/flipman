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
    "${CMAKE_CURRENT_SOURCE_DIR}/render/*.mm"
)
source_group("Header Files\\render" FILES ${sdk_render_headers})
source_group("Source Files\\render" FILES ${sdk_render_sources})

# widgets
file(GLOB sdk_widgets_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/widgets/*.h")
file(GLOB sdk_widgets_sources 
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.mm"
)
source_group("Header Files\\widgets" FILES ${sdk_widgets_headers})
source_group("Source Files\\widgets" FILES ${sdk_widgets_sources})

# Internal headers are visible in the IDE but never form part of the public API.
file(GLOB_RECURSE sdk_private_headers CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/av/private/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/private/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/private/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/plugins/*/private/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/render/private/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/private/*.h"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Private Headers" FILES ${sdk_private_headers})

# resources
file(GLOB sdk_root_headers
     "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/*.h")

file(GLOB sdk_resources
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/*.qss"
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/ui/*.qrc"
)

file(GLOB sdk_glsl_resources
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/glsl/*.glsl"
)

file(GLOB sdk_metal_resources
     "${CMAKE_CURRENT_SOURCE_DIR}/resources/metal/*.metal"
)

source_group("Resources" FILES ${sdk_resources})
source_group("Resources\\glsl" FILES ${sdk_glsl_resources})
source_group("Resources\\metal" FILES ${sdk_metal_resources})

# target definition
add_library(${sdk_name} SHARED
    ${sdk_private_headers}
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
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/configs/studio-config-v4.0.0_aces-v2.0_ocio-v2.5.ocio"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/configs/LICENSE"
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_export_header(${sdk_name}
    BASE_NAME FLIPMANSDK
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/include/flipmansdk/flipmanexport.h"
)

target_compile_features(${sdk_name} PUBLIC cxx_std_17)
set_target_properties(${sdk_name} PROPERTIES
    OBJCXX_STANDARD 17
    OBJCXX_STANDARD_REQUIRED YES
)

# Compile each public header independently, without the SDK's private include
# directories or precompiled headers. Build explicitly with --target flipmansdk_headers.
file(GLOB_RECURSE sdk_public_headers CONFIGURE_DEPENDS
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/flipmansdk/*.h"
)
list(APPEND sdk_public_headers "flipmansdk/flipmanexport.h")
list(REMOVE_DUPLICATES sdk_public_headers)
set(sdk_header_checks)
foreach(header IN LISTS sdk_public_headers)
    set(check_source "${CMAKE_CURRENT_BINARY_DIR}/header_checks/${header}.cpp")
    file(GENERATE OUTPUT "${check_source}" CONTENT "#include <${header}>\n")
    list(APPEND sdk_header_checks "${check_source}")
endforeach()
add_library(flipmansdk_headers OBJECT EXCLUDE_FROM_ALL ${sdk_header_checks})
target_link_libraries(flipmansdk_headers PRIVATE ${sdk_name})
set_target_properties(flipmansdk_headers PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

target_link_libraries(${sdk_name}
    PUBLIC
        Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::GuiPrivate Qt6::ShaderTools Qt6::Widgets
    PRIVATE
        Qt6::ShaderToolsPrivate
        flipman_glsl_parser
        OpenImageIO::OpenImageIO
        OpenColorIO::OpenColorIO
        OpenImageIO::OpenImageIO_Util
        "-framework CoreFoundation"
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework AppKit"
        "-framework Metal"
        "-framework QuartzCore"
)

if(FLIPMAN_BUILD_BRAW)
    find_package(BlackmagicRaw REQUIRED)
    target_link_libraries(${sdk_name} PRIVATE BlackmagicRaw::Dispatch)
    target_compile_definitions(${sdk_name} PRIVATE FLIPMAN_HAS_BRAW=1)
endif()
