#include <gtest/gtest.h>
#include <QApplication>
#include <iostream>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    std::cout << "\n=== LOGITUNE HARDWARE TESTS ===\n"
              << "Requires: MX Master 3S connected via Bolt receiver\n"
              << "These tests send real HID++ commands to the device.\n"
              << "Device state will be restored after each test.\n\n"
              << "Press Enter to continue, Ctrl+C to abort...\n";
    std::cin.get();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
