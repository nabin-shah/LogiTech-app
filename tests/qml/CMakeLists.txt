qt_add_executable(logitune-qml-tests
    tst_qml_main.cpp
)

target_link_libraries(logitune-qml-tests PRIVATE
    logitune-app-lib
    logitune-app-libplugin
    Qt6::QuickTest
    Qt6::Quick
    Qt6::Widgets
)

target_compile_definitions(logitune-qml-tests PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME logitune-qml-tests
    COMMAND logitune-qml-tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(logitune-qml-tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
