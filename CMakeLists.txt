cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" VOGUE_VERSION LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt")
project(vogue VERSION "${VOGUE_VERSION}" LANGUAGES C)

add_library(vogue
    src/context.c
    src/button.c
    src/accessibility.c
)
add_library(vogue::vogue ALIAS vogue)

target_include_directories(vogue PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(vogue PRIVATE c_std_11)
set_target_properties(vogue PROPERTIES
    C_EXTENSIONS OFF
    C_VISIBILITY_PRESET hidden
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(vogue PUBLIC VOGUE_SHARED PRIVATE VOGUE_BUILD)
endif()
