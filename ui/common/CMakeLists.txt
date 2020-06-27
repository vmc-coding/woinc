include(woincSetupCompilerOptions)

set(WOINC_UI_COMMON_SOURCES
    types_to_string.cc
)

add_library(woinc_ui_common OBJECT ${WOINC_UI_COMMON_SOURCES})

woincSetupCompilerOptions(woinc_ui_common)

get_target_property(WOINC_INCLUDE_DIRS woinc::core INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(woinc_ui_common PRIVATE ${WOINC_INCLUDE_DIRS})

