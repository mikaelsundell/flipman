# Copyright 2022-present Contributors to the flipman project.
# SPDX-License-Identifier: BSD-3-Clause
# https://github.com/mikaelsundell/flipman

function(generate_qt_shaders target_name shader_sources qrc_input_path qsb_output_var qrc_output_var)
    set(qsb_targets --qt6 --glsl "100es,120,300es")
    if (APPLE)
        list(APPEND qsb_targets --msl 12)
    elseif(WIN32)
        list(APPEND qsb_targets --hlsl 50)
    endif()

    function(generate_qrc_entry alias_path target_path out_var)
        set(entry "        <file alias=\"${alias_path}\">${target_path}</file>\n")
        set(${out_var} "${entry}" PARENT_SCOPE)
    endfunction()

    set(shader_qsb_files "")
    set(qrc_shader_entries "")

    foreach(shader ${shader_sources})
        get_filename_component(shader_name ${shader} NAME)
        set(output_qsb "${CMAKE_CURRENT_BINARY_DIR}/${shader_name}.qsb")
        add_custom_command(
            OUTPUT ${output_qsb}
            COMMAND Qt6::qsb ${shader} ${qsb_targets} -o ${output_qsb}
            MAIN_DEPENDENCY ${shader}
            COMMENT "Compiling shader ${shader_name} -> ${output_qsb}"
            VERBATIM
        )
        list(APPEND shader_qsb_files ${output_qsb})
        generate_qrc_entry("${shader_name}.qsb" "${output_qsb}" entry)
        string(APPEND qrc_shader_entries ${entry})
    endforeach()

    if(NOT TARGET ${target_name}_shaders)
        add_custom_target(${target_name}_shaders ALL DEPENDS ${shader_qsb_files})
    endif()

    get_filename_component(qrc_input_name ${qrc_input_path} NAME_WE)
    set(qrc_output "${CMAKE_CURRENT_BINARY_DIR}/${qrc_input_name}.qrc")

    configure_file(${qrc_input_path} ${qrc_output} @ONLY)
    qt_add_resources(local_qrc_output ${qrc_output})
    set(${qsb_output_var} ${shader_qsb_files} PARENT_SCOPE)
    set(${qrc_output_var} ${local_qrc_output} PARENT_SCOPE)
endfunction()
