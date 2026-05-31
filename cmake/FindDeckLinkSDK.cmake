# FindDeckLinkSDK.cmake
#
# Usage:
#   set(DeckLinkSDK_ROOT "/path/to/Blackmagic DeckLink SDK")
#   find_package(DeckLinkSDK REQUIRED)
#
# Provides:
#   DeckLinkSDK::DeckLinkSDK
#   DeckLinkSDK_FOUND
#   DeckLinkSDK_INCLUDE_DIR
#   DeckLinkSDK_DISPATCH_SOURCE

include(FindPackageHandleStandardArgs)

set(DeckLinkSDK_ROOT "" CACHE PATH "Root folder of the Blackmagic DeckLink SDK")

find_path(DeckLinkSDK_INCLUDE_DIR
    NAMES DeckLinkAPI.h
    HINTS
        ${DeckLinkSDK_ROOT}
        $ENV{DeckLinkSDK_ROOT}
    PATH_SUFFIXES
        Mac/include
        include
)

find_file(DeckLinkSDK_DISPATCH_SOURCE
    NAMES DeckLinkAPIDispatch.cpp
    HINTS
        ${DeckLinkSDK_ROOT}
        $ENV{DeckLinkSDK_ROOT}
    PATH_SUFFIXES
        Mac/include
        include
)

find_package_handle_standard_args(DeckLinkSDK
    REQUIRED_VARS
        DeckLinkSDK_INCLUDE_DIR
        DeckLinkSDK_DISPATCH_SOURCE
)

if(DeckLinkSDK_FOUND AND NOT TARGET DeckLinkSDK::DeckLinkSDK)
    add_library(DeckLinkSDK STATIC
        ${DeckLinkSDK_DISPATCH_SOURCE}
    )

    add_library(DeckLinkSDK::DeckLinkSDK ALIAS DeckLinkSDK)

    target_include_directories(DeckLinkSDK
        PUBLIC
            ${DeckLinkSDK_INCLUDE_DIR}
    )

    target_compile_features(DeckLinkSDK
        PUBLIC
            cxx_std_17
    )

    if(APPLE)
        target_link_libraries(DeckLinkSDK
            PUBLIC
                "-framework CoreFoundation"
                "-framework IOKit"
        )
    endif()
endif()

mark_as_advanced(
    DeckLinkSDK_INCLUDE_DIR
    DeckLinkSDK_DISPATCH_SOURCE
)