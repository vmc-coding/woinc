# compile feature cxx_std_14 had been introduced with 3.8
cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

mark_as_advanced(FORCE WOINC_VERSION_MAJOR)
mark_as_advanced(FORCE WOINC_VERSION_MINOR)

set(WOINC_VERSION_MAJOR    0 CACHE STRING "Major version of woinc." FORCE)
set(WOINC_VERSION_MINOR 9999 CACHE STRING "Minor version of woinc." FORCE)
set(WOINC_VERSION ${WOINC_VERSION_MAJOR}.${WOINC_VERSION_MINOR})

project(woinc VERSION ${WOINC_VERSION} LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

option(WOINC_BUILD_LIB    "Build woinc library" ON)
option(WOINC_BUILD_LIBUI  "Build woincui library" ON)
option(WOINC_BUILD_UI_CLI "Build woincmd" ON)
option(WOINC_BUILD_UI_QT  "Build woincqt" ON)
option(WOINC_CLI_COMMANDS "Add woinc's own commands to the cli client" OFF)
option(WOINC_EXPOSE_FULL_STRUCTURES
    "Expose full GUI-RPC data structures. When disabled, only the
    structures the woinc UIs use will be exposed. This reduces the
    binary size, memory footprint and processing time for parsing." OFF)
option(WOINC_BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(WOINC_ENABLE_COVERAGE "Enable gcc's coverage reporting" OFF)
option(WOINC_ENABLE_SANITIZER "Enable gcc's AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(WOINC_BUILD_SHARED_LIBS)
    mark_as_advanced(FORCE WOINC_SONAME_VERSION)
    set(WOINC_SONAME_VERSION ${WOINC_VERSION_MAJOR} CACHE STRING
        "The soname of the shared woinc library." FORCE)
endif()

include(GNUInstallDirs)

# Offer the user the choice of overriding the installation directories
# From https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/How-to-create-a-ProjectConfig.cmake-file
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR ${CMAKE_INSTALL_BINDIR} CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Installation directory for header files")

if(WIN32 AND NOT CYGWIN)
    set(DEF_INSTALL_CMAKE_DIR CMake)
else()
    set(DEF_INSTALL_CMAKE_DIR lib/cmake/woinc)
endif()

set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

### Find external dependencies ###

find_package(Threads REQUIRED)

### add a 'doc' target using doxygen to create the docu ###

#find_package(Doxygen)
#
#if(DOXYGEN_FOUND)
#    # enable generating fancy diagrams with dot if available
#    set(HAVE_DOT "NO")
#    if(DOXYGEN_DOT_FOUND)
#        set(HAVE_DOT "YES")
#    endif ()
#
#    # configure doxygen
#    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
#
#    # add the 'doc' tag
#    add_custom_target(doc
#        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
#        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#        COMMENT "Generating documentation with Doxygen" VERBATIM)
#endif()

### subprojects ###

enable_testing()

if(WOINC_BUILD_LIB)
    add_subdirectory(lib)
else()
    find_package(woinc REQUIRED)
endif()

set(NEED_WOINC_LIBUI ${WOINC_BUILD_UI_QT})

if(WOINC_BUILD_LIBUI)
    add_subdirectory(libui)
elseif(NEED_WOINC_LIBUI)
    find_package(woinc REQUIRED NAMES woincui)
endif()

add_subdirectory(ui)
