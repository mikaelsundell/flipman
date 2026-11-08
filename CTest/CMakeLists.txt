# SPDX-License-Identifier: BSD-3-Clause

set(FLIPMAN_TEST_DATA_DIR "${PROJECT_SOURCE_DIR}/data" CACHE PATH "External SDK integration fixtures")
option(FLIPMAN_REQUIRE_TEST_DATA "Fail instead of skipping when external fixtures are missing" OFF)
option(FLIPMAN_ENABLE_TIMING_TESTS "Enable the legacy wall-clock timer benchmark" OFF)
option(FLIPMAN_FULL_MEDIA_TESTS "Run full legacy media workloads instead of bounded samples" OFF)

add_executable(flipmansdk_tests main.cpp sdk.cpp sdk.h)
target_link_libraries(flipmansdk_tests PRIVATE flipmansdk)

set(test_data_args "${FLIPMAN_TEST_DATA_DIR}")
if(FLIPMAN_REQUIRE_TEST_DATA)
    list(APPEND test_data_args --require-data)
endif()

function(sdk_test case labels timeout)
    add_test(NAME flipman.sdk.${case}
        COMMAND ${CMAKE_COMMAND} -E env
            "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
            QT_QPA_PLATFORM=offscreen
            "FLIPMAN_FULL_MEDIA_TESTS=$<BOOL:${FLIPMAN_FULL_MEDIA_TESTS}>"
            $<TARGET_FILE:flipmansdk_tests> ${case} ${test_data_args})
    set_tests_properties(flipman.sdk.${case} PROPERTIES
        LABELS "sdk;${labels}" TIMEOUT ${timeout} SKIP_RETURN_CODE 77)
endfunction()

foreach(case IN ITEMS file fps time time_range smpte image_classification image_nv12_layout
        image_packed_layout image_uint16 image_double image_planar image_interleaved timeline media_state shader plugin_fx shader_contract)
    sdk_test(${case} "native;cpu" 30)
endforeach()
foreach(case IN ITEMS clip media plugin_container plugin_image plugin_registry image_nv12
        image_average image_threaded)
    sdk_test(${case} "native;integration;data" 120)
endforeach()
foreach(case IN ITEMS render_offscreen render_roundtrip)
    sdk_test(${case} "native;integration;data;gpu" 120)
endforeach()
set_tests_properties(flipman.sdk.image_nv12 PROPERTIES LABELS "sdk;native;integration;data;gpu")
sdk_test(timer "native;timing" 120)
if(FLIPMAN_ENABLE_TIMING_TESTS)
    set_tests_properties(flipman.sdk.timer PROPERTIES DISABLED FALSE)
else()
    set_tests_properties(flipman.sdk.timer PROPERTIES DISABLED TRUE)
endif()

if(TARGET flipman_python)
    foreach(case IN ITEMS time core image media render)
        add_test(NAME flipman.python.${case}
            COMMAND ${CMAKE_COMMAND} -E env
                "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
                QT_QPA_PLATFORM=offscreen
                "PYTHONPATH=$<TARGET_FILE_DIR:flipman_python>"
                ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/python/test_sdk.py" ${case})
        set_tests_properties(flipman.python.${case} PROPERTIES LABELS "python;sdk;cpu" TIMEOUT 60)
    endforeach()
    set_tests_properties(flipman.python.render PROPERTIES LABELS "python;sdk;gpu")
    if(APPLE)
        add_test(NAME flipman.python.runtime_guard
            COMMAND ${CMAKE_COMMAND} -E env --unset=DYLD_IMAGE_SUFFIX
                QT_QPA_PLATFORM=offscreen "PYTHONPATH=$<TARGET_FILE_DIR:flipman_python>"
                ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/python/test_runtime_guard.py"
                "$<CONFIG>")
        set_tests_properties(flipman.python.runtime_guard PROPERTIES LABELS "python;sdk;cpu" TIMEOUT 30)
    endif()
    add_test(NAME flipman.python.launcher
        COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen
            "$<TARGET_FILE_DIR:flipman_python>/flipman-python"
            -c "import flipman; runtime = flipman.Runtime(); runtime.processEvents()")
    set_tests_properties(flipman.python.launcher PROPERTIES LABELS "python;sdk;cpu" TIMEOUT 30)
endif()
add_custom_target(flipman_tests DEPENDS flipmansdk_tests flipmansdk_headers)
if(TARGET flipman_python)
    add_dependencies(flipman_tests flipman_python)
endif()

add_executable(flipman_profile_tests profile.cpp)
target_link_libraries(flipman_profile_tests PRIVATE flipmansdk)
foreach(case IN ITEMS recording logging unset empty enabled)
    add_test(NAME flipman.profile.${case}
        COMMAND ${CMAKE_COMMAND} -E env
            "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
            --unset=FLIPMAN_PROFILE_REPORT --unset=QT_LOGGING_RULES
            $<TARGET_FILE:flipman_profile_tests> ${case})
    set_tests_properties(flipman.profile.${case} PROPERTIES LABELS "sdk;native;cpu;profile" TIMEOUT 30)
endforeach()
add_dependencies(flipman_tests flipman_profile_tests)

add_executable(flipman_shaderparser_tests shaderparser.cpp)
target_link_libraries(flipman_shaderparser_tests PRIVATE flipmansdk)
add_test(NAME flipman.sdk.shader_parser
    COMMAND ${CMAKE_COMMAND} -E env
        "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
        $<TARGET_FILE:flipman_shaderparser_tests>)
set_tests_properties(flipman.sdk.shader_parser PROPERTIES LABELS "sdk;native;cpu" TIMEOUT 30)
add_dependencies(flipman_tests flipman_shaderparser_tests)

add_executable(flipman_brawreader_tests brawreader.cpp)
target_link_libraries(flipman_brawreader_tests PRIVATE flipmansdk)
add_dependencies(flipman_tests flipman_brawreader_tests)
set(braw_test_environment
    "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>")
if(FLIPMAN_BUILD_BRAW)
    target_compile_definitions(flipman_brawreader_tests PRIVATE FLIPMAN_TEST_BRAW_ENABLED=1)
    get_filename_component(braw_sdk_mac_dir "${BlackmagicRaw_INCLUDE_DIR}" DIRECTORY)
    set(FLIPMAN_BRAW_TEST_RUNTIME_DIR "${braw_sdk_mac_dir}/Libraries" CACHE PATH "BRAW framework parent for tests")
    set(FLIPMAN_BRAW_TEST_FILE "" CACHE FILEPATH "External BRAW decode fixture")
    list(APPEND braw_test_environment "FLIPMAN_BRAW_RUNTIME_DIR=${FLIPMAN_BRAW_TEST_RUNTIME_DIR}")
    if(FLIPMAN_REQUIRE_TEST_DATA)
        list(APPEND braw_test_environment "FLIPMAN_REQUIRE_BRAW_DATA=1")
    endif()
    add_test(NAME flipman.sdk.braw_decode COMMAND ${CMAKE_COMMAND} -E env ${braw_test_environment}
        $<TARGET_FILE:flipman_brawreader_tests> decode "${FLIPMAN_BRAW_TEST_FILE}")
    set_tests_properties(flipman.sdk.braw_decode PROPERTIES LABELS "sdk;native;integration;data;braw" TIMEOUT 120 SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
endif()
add_test(NAME flipman.sdk.braw_state COMMAND ${CMAKE_COMMAND} -E env ${braw_test_environment}
    $<TARGET_FILE:flipman_brawreader_tests> state)
set_tests_properties(flipman.sdk.braw_state PROPERTIES LABELS "sdk;native;cpu;braw" TIMEOUT 30)
if(FLIPMAN_BUILD_BRAW AND TARGET flipman_python)
    add_test(NAME flipman.python.braw COMMAND ${CMAKE_COMMAND} -E env ${braw_test_environment}
        QT_QPA_PLATFORM=offscreen "PYTHONPATH=$<TARGET_FILE_DIR:flipman_python>"
        ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/python/test_braw.py" "${FLIPMAN_BRAW_TEST_FILE}")
    set_tests_properties(flipman.python.braw PROPERTIES LABELS "sdk;python;integration;data;braw" TIMEOUT 120 SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
endif()

add_executable(flipman_glsl_container_tests glslcontainer.cpp)
target_compile_features(flipman_glsl_container_tests PRIVATE cxx_std_11)
target_include_directories(flipman_glsl_container_tests PRIVATE "${PROJECT_SOURCE_DIR}/3rdparty/glsl-parser")
set_target_properties(flipman_glsl_container_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
if(APPLE)
    # Match Xcode Debug's checked libc++, including in Makefile builds.
    target_compile_definitions(flipman_glsl_container_tests PRIVATE
        _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
endif()
add_test(NAME flipman.sdk.glsl_container COMMAND flipman_glsl_container_tests)
set_tests_properties(flipman.sdk.glsl_container PROPERTIES LABELS "sdk;native;cpu" TIMEOUT 10)
add_dependencies(flipman_tests flipman_glsl_container_tests)

add_executable(flipman_render_channels_tests renderchannels.cpp)
target_link_libraries(flipman_render_channels_tests PRIVATE flipmansdk)
add_test(NAME flipman.sdk.render_channels COMMAND ${CMAKE_COMMAND} -E env
    "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
    QT_QPA_PLATFORM=offscreen $<TARGET_FILE:flipman_render_channels_tests>)
set_tests_properties(flipman.sdk.render_channels PROPERTIES LABELS "sdk;native;integration;gpu" TIMEOUT 60)
add_dependencies(flipman_tests flipman_render_channels_tests)

set(channel_fixture_environment)
if(FLIPMAN_REQUIRE_TEST_DATA)
    list(APPEND channel_fixture_environment FLIPMAN_REQUIRE_CHANNEL_DATA=1)
endif()
add_test(NAME flipman.sdk.quicktime_channels COMMAND ${CMAKE_COMMAND} -E env
    "DYLD_IMAGE_SUFFIX=$<$<AND:$<PLATFORM_ID:Darwin>,$<CONFIG:Debug>>:_debug>"
    QT_QPA_PLATFORM=offscreen ${channel_fixture_environment} $<TARGET_FILE:flipman_render_channels_tests>
    "${FLIPMAN_TEST_DATA_DIR}/quicktime/iphone17pro film.mov")
set_tests_properties(flipman.sdk.quicktime_channels PROPERTIES
    LABELS "sdk;native;integration;gpu;data" TIMEOUT 60 SKIP_RETURN_CODE 77)
