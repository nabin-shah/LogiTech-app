add_executable(logitune-hw-tests
    hw_test_main.cpp
    test_hw_thumbwheel.cpp
    test_hw_device.cpp
    test_hw_features.cpp
    test_hw_settings_readback.cpp
    test_hw_sleep_wake.cpp
)
target_include_directories(logitune-hw-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(logitune-hw-tests PRIVATE logitune-core Qt6::Test Qt6::Widgets GTest::gtest)
