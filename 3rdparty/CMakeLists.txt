# SPDX-License-Identifier: BSD-3-Clause

add_library(flipman_glsl_parser STATIC
    glsl-parser/ast.cpp
    glsl-parser/lexer.cpp
    glsl-parser/parser.cpp
    glsl-parser/util.cpp
)
target_compile_features(flipman_glsl_parser PUBLIC cxx_std_11)
target_include_directories(flipman_glsl_parser SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/glsl-parser")
set_target_properties(flipman_glsl_parser PROPERTIES
    AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
    CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
)
