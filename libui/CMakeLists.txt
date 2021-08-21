project(libwoincui VERSION ${WOINC_VERSION} LANGUAGES CXX)

include(woincSetupCompilerOptions)

### collect header and implementation files ###

set(WOINC_LIBUI_INTERFACE
    include/woinc/ui/controller.h
    include/woinc/ui/defs.h
    include/woinc/ui/error.h
    include/woinc/ui/handler.h
)

set(WOINC_LIBUI_HEADERS
    src/client.h
    src/configuration.h
    src/handler_registry.h
    src/host_controller.h
    src/job_queue.h
    src/jobs.h
    src/periodic_tasks_scheduler.h
)

set(WOINC_LIBUI_SOURCES
    src/client.cc
    src/configuration.cc
    src/controller.cc
    src/handler_registry.cc
    src/host_controller.cc
    src/job_queue.cc
    src/jobs.cc
    src/periodic_tasks_scheduler.cc
)

### create woincui library ###

if(WOINC_BUILD_SHARED_LIBRARY)
    add_library(woincui SHARED ${WOINC_LIBUI_INTERFACE} ${WOINC_LIBUI_HEADERS} ${WOINC_LIBUI_SOURCES})
    set_target_properties(woincui PROPERTIES VERSION ${WOINC_VERSION} SOVERSION ${WOINC_SONAME_VERSION})
else()
    add_library(woincui STATIC ${WOINC_LIBUI_INTERFACE} ${WOINC_LIBUI_HEADERS} ${WOINC_LIBUI_SOURCES})
endif()

woincSetupCompilerOptions(woincui)

set_target_properties(woincui PROPERTIES PUBLIC_HEADER "${WOINC_LIBUI_INTERFACE}")

target_include_directories(woincui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

### link external libs against woincui

target_link_libraries(woincui PUBLIC woinc::core PRIVATE Threads::Threads)

### install the woincui library ###

set_target_properties(woincui PROPERTIES EXPORT_NAME ui)

install(TARGETS woincui
    EXPORT woincui-config
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
    PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/woinc/ui"
    PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

install(EXPORT woincui-config
    DESTINATION ${INSTALL_CMAKE_DIR}
    NAMESPACE woinc::
    COMPONENT dev)

# make the project importable from the build directory
export(TARGETS woincui FILE woincuiConfig.cmake)
add_library(woinc::ui ALIAS woincui)

add_subdirectory(profiling EXCLUDE_FROM_ALL)
