project(woinccmd VERSION ${WOINC_VERSION} LANGUAGES CXX)

include(woincSetupCompilerOptions)

set(WOINCCMD_SOURCES
    main.cc
)

add_executable(woinccmd ${WOINCCMD_SOURCES})
target_link_libraries(woinccmd PRIVATE $<TARGET_OBJECTS:woinc_ui_common> woinc::core)

woincSetupCompilerOptions(woinccmd)

if(WOINC_CLI_COMMANDS)
    target_compile_definitions(woinccmd PRIVATE WOINC_CLI_COMMANDS)
endif()

install(TARGETS woinccmd DESTINATION ${INSTALL_BIN_DIR}
    PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
