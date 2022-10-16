project(libwoinc VERSION ${WOINC_VERSION} LANGUAGES CXX)

include(woincSetupCompilerOptions)

### find external dependencies ###

find_package(pugixml REQUIRED)

### configure types ###

if(WOINC_EXPOSE_FULL_STRUCTURES)
    set(WOINC_EXPOSE_FULL_STRUCTURES "1")
else()
    set(WOINC_EXPOSE_FULL_STRUCTURES "0")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/woinc/types.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/woinc/types.h
    @ONLY
)

### configure version ###

# used when exchanging versions with BOINC
mark_as_advanced(FORCE WOINC_BOINC_VERSION_MAJOR)
mark_as_advanced(FORCE WOINC_BOINC_VERSION_MINOR)
mark_as_advanced(FORCE WOINC_BOINC_VERSION_RELEASE)

set(WOINC_BOINC_VERSION_MAJOR 7 CACHE STRING "Major version of BOINC." FORCE)
set(WOINC_BOINC_VERSION_MINOR 20 CACHE STRING "Minor version of BOINC." FORCE)
set(WOINC_BOINC_VERSION_RELEASE 2 CACHE STRING "Release version of BOINC." FORCE)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cc.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/version.cc
)

### collect header and implementation files ###

set(WOINC_LIB_INTERFACE
    include/woinc/defs.h
    include/woinc/rpc_command.h
    include/woinc/rpc_connection.h
    include/woinc/version.h

    ${CMAKE_CURRENT_BINARY_DIR}/include/woinc/types.h
)

set(WOINC_LIB_HEADERS
    src/md5.h
    src/rpc_parsing.h
    src/socket.h
    src/visibility.h
    src/xml.h
)

set(WOINC_LIB_SOURCES
    src/md5.cc
    src/rpc_command.cc
    src/rpc_connection.cc
    src/rpc_parsing.cc
    src/socket_posix.cc
    src/types.cc
    src/xml.cc

    ${CMAKE_CURRENT_BINARY_DIR}/src/version.cc
)

### create woinc library ###

if(WOINC_BUILD_SHARED_LIBRARY)
    add_library(woinc SHARED ${WOINC_LIB_INTERFACE} ${WOINC_LIB_HEADERS} ${WOINC_LIB_SOURCES})
    set_target_properties(woinc PROPERTIES VERSION ${WOINC_VERSION} SOVERSION ${WOINC_SONAME_VERSION})
else()
    add_library(woinc STATIC ${WOINC_LIB_INTERFACE} ${WOINC_LIB_HEADERS} ${WOINC_LIB_SOURCES})
endif()

woincSetupCompilerOptions(woinc)

if(WOINC_EXPOSE_FULL_STRUCTURES)
    target_compile_definitions(woinc PRIVATE WOINC_EXPOSE_FULL_STRUCTURES)
endif()

set_target_properties(woinc PROPERTIES PUBLIC_HEADER "${WOINC_LIB_INTERFACE}")

target_include_directories(woinc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(woinc PRIVATE pugixml Threads::Threads)

### test the library ###

add_subdirectory(tests EXCLUDE_FROM_ALL)

set(WOINC_ALL_TEST_TARGETS ${WOINC_ALL_TEST_TARGETS} PARENT_SCOPE)
set(WOINC_ALL_MANUAL_TEST_TARGETS ${WOINC_ALL_MANUAL_TEST_TARGETS} PARENT_SCOPE)

### install the woinc library as target woinc::core ###

set_target_properties(woinc PROPERTIES EXPORT_NAME core)

install(TARGETS woinc
    EXPORT woinc-config
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
    PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/woinc"
    PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

install(EXPORT woinc-config
    DESTINATION ${INSTALL_CMAKE_DIR}
    NAMESPACE woinc::
    COMPONENT dev)

# make the project importable from the build directory
export(TARGETS woinc FILE woincConfig.cmake)
add_library(woinc::core ALIAS woinc)

