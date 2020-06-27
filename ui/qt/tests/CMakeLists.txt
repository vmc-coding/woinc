include(woincSetupCompilerOptions)

set(WOINC_UI_QT_TESTS
    tab_model_updater_test
)

foreach(testname IN LISTS WOINC_UI_QT_TESTS)
    add_executable(${testname} ${testname}.cc)
    woincSetupCompilerOptions(${testname})
    target_link_libraries(${testname} PRIVATE Qt5::Widgets Qt5::Test)
    add_test(${testname} ${testname})
endforeach()

# add custom targets

add_custom_target(ui-qt-tests DEPENDS
    ${WOINC_UI_QT_TESTS}
    COMMENT "Build test cases for the Qt UI" VERBATIM)

add_custom_target(check-ui-qt COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ui-qt-tests
    COMMENT "Run test cases for the Qt UI" VERBATIM)
