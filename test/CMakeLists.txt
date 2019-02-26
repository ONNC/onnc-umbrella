file(GLOB regression_tests RELATIVE ${CMAKE_CURRENT_LIST_DIR}/regression regression/*)

foreach(regression_test IN LISTS regression_tests)
    add_test(
        NAME ${regression_test}
        COMMAND bash "${CMAKE_CURRENT_LIST_DIR}/regression/${regression_test}/main.sh"
    )
    set_tests_properties(${regression_test} PROPERTIES
        ENVIRONMENT "PATH=$ENV{PATH}:${ONNC_UMBRELLA_ROOT}/install-${build_mode_for_command}${CMAKE_INSTALL_PREFIX}/bin"
    )
endforeach(regression_test)