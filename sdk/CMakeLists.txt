# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

set (sdk_name "${project_name}Sdk")

include(${CMAKE_SOURCE_DIR}/cmake/qtutils.cmake)

file (GLOB sdk_av_sources
    "${CMAKE_SOURCE_DIR}/sdk/av/*.cpp"
    "${CMAKE_SOURCE_DIR}/sdk/av/*.mm"
    "${CMAKE_SOURCE_DIR}/sdk/include/av/*.h"
)

file (GLOB sdk_core_sources
    "${CMAKE_SOURCE_DIR}/sdk/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/sdk/core/*.mm"
    "${CMAKE_SOURCE_DIR}/sdk/include/core/*.h"
)

file (GLOB sdk_plugins_sources
    "${CMAKE_SOURCE_DIR}/sdk/plugins/*.cpp"
    "${CMAKE_SOURCE_DIR}/sdk/plugins/*.mm"
    "${CMAKE_SOURCE_DIR}/sdk/include/plugins/*.h"
)

file (GLOB sdk_widgets_sources
    "${CMAKE_SOURCE_DIR}/sdk/widgets/*.cpp"
    "${CMAKE_SOURCE_DIR}/sdk/widgets/*.mm"
    "${CMAKE_SOURCE_DIR}/sdk/include/widgets/*.h"
)

file (GLOB sdk_shaders_sources
    "${CMAKE_SOURCE_DIR}/sdk/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/sdk/shaders/*.vert"
)

compile_qt_shaders(${sdk_name} "${sdk_shaders_sources}" sdk_qsb_shaders sdk_shaders_qsb_sources)

source_group ("av Files" FILES ${sdk_av_sources})
source_group ("core Files" FILES ${sdk_core_sources})
source_group ("plugins Files" FILES ${sdk_plugins_sources})
source_group ("widgets Files" FILES ${sdk_widgets_sources})
source_group ("qsb Files" FILES ${sdk_shaders_qsb_sources})

add_library (${sdk_name} SHARED
    ${sdk_av_sources}
    ${sdk_core_sources}
    ${sdk_plugins_sources}
    ${sdk_widgets_sources}
    ${sdk_shaders_qsb_sources}
)

target_include_directories (${sdk_name} PUBLIC ${CMAKE_SOURCE_DIR}/sdk/include)
target_link_libraries (${sdk_name}
    Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::GuiPrivate Qt6::Widgets
    "-framework CoreFoundation"
    "-framework AVFoundation"
    "-framework CoreMedia"
    "-framework CoreVideo"
    "-framework AppKit"
)

add_dependencies (${sdk_name} ${sdk_name}_shaders)
