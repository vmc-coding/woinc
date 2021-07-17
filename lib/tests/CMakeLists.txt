include(woincSetupCompilerOptions)

# create the command tests

#set(WOINC_CMD_TESTS
    #authorize_cmd_test
    #exchange_versions_cmd_test
    #get_cc_status_cmd_test
    #get_results_cmd_test
#)

#foreach(testname IN LISTS WOINC_CMD_TESTS)
    #add_executable(${testname} commands/${testname}.cc test.cc ../src/xml.cc)
    #woincSetupCompilerOptions(${testname})
    #target_link_libraries(${testname} PRIVATE woinc pugixml)
    #add_test(${testname} ${testname})
#endforeach()

# create other tests

add_executable(md5_tests md5_tests.cc test.cc ../src/md5.cc)
woincSetupCompilerOptions(md5_tests)

add_executable(xml_tests xml_tests.cc test.cc ../src/xml.cc)
woincSetupCompilerOptions(xml_tests)
target_link_libraries(xml_tests PRIVATE pugixml)

set(WOINC_TESTS
    md5_tests
    xml_tests
)

add_executable(manual_posix_socket_tests test.cc manual/posix_socket_tests.cc ../src/socket_posix.cc)
woincSetupCompilerOptions(manual_posix_socket_tests)

set(MANUAL_WOINC_TESTS
    manual_posix_socket_tests
)

foreach(testname IN LISTS WOINC_TESTS)
    add_test(${testname} ${testname})
endforeach()

# add custom targets

add_custom_target(tests DEPENDS
    ${WOINC_TESTS}
    ${WOINC_CMD_TESTS}
    COMMENT "Build test cases" VERBATIM)

add_custom_target(manual-tests DEPENDS
    ${MANUAL_WOINC_TESTS}
    COMMENT "Build manual test cases" VERBATIM)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS tests
    COMMENT "Run test cases" VERBATIM)
