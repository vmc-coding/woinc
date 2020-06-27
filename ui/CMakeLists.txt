if (WOINC_BUILD_UI_CLI OR WOINC_BUILD_UI_QT)
    include_directories(.)
    add_subdirectory(common)
endif()

if (WOINC_BUILD_UI_CLI)
    add_subdirectory(cli)
endif()

if (WOINC_BUILD_UI_QT)
    add_subdirectory(qt)
endif()
