#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>
#include <iostream>
#include "hw/HardwareFixture.h"

using namespace logitune;
using namespace logitune::test;

TEST_F(HardwareFixture, DeviceReportsName) {
    EXPECT_FALSE(m_dm.deviceName().isEmpty());
    std::cout << "Device: " << m_dm.deviceName().toStdString() << "\n";
}

TEST_F(HardwareFixture, BatteryLevelIsReasonable) {
    int level = m_dm.batteryLevel();
    EXPECT_GE(level, 0);
    EXPECT_LE(level, 100);
    std::cout << "Battery: " << level << "%\n";
}

TEST_F(HardwareFixture, DPIReadBackMatchesSet) {
    m_dm.setDPI(1200);
    int actual = m_dm.currentDPI();
    EXPECT_NEAR(actual, 1200, 50);
    std::cout << "DPI set: 1200, read back: " << actual << "\n";
}

TEST_F(HardwareFixture, SmartShiftReadBackMatchesSet) {
    m_dm.setSmartShift(true, 75);
    EXPECT_TRUE(m_dm.smartShiftEnabled());
    EXPECT_EQ(m_dm.smartShiftThreshold(), 75);
}

TEST_F(HardwareFixture, SurvivesReconnect) {
    std::cout << ">>> Turn off the mouse, wait 2s, turn back on (15 seconds)...\n";

    QSignalSpy disconnSpy(&m_dm, &DeviceManager::deviceDisconnected);
    QSignalSpy reconnSpy(&m_dm, &DeviceManager::deviceSetupComplete);

    QEventLoop loop;
    QObject::connect(&m_dm, &DeviceManager::deviceSetupComplete, &loop, &QEventLoop::quit);
    QTimer::singleShot(15000, &loop, &QEventLoop::quit);
    loop.exec();

    std::cout << "Disconnect signals: " << disconnSpy.count()
              << ", Reconnect signals: " << reconnSpy.count() << "\n";

    if (disconnSpy.count() > 0 && reconnSpy.count() > 0) {
        EXPECT_TRUE(m_dm.deviceConnected());
        EXPECT_FALSE(m_dm.deviceName().isEmpty());
        std::cout << "Reconnect OK: " << m_dm.deviceName().toStdString() << "\n";
    } else if (disconnSpy.count() > 0) {
        GTEST_SKIP() << "Disconnect detected but no reconnect within 15 seconds — turn mouse back on faster";
    } else {
        GTEST_SKIP() << "No disconnect detected — turn mouse off during the test window";
    }
}

TEST_F(HardwareFixture, BatteryNotificationArrivesOrganically) {
    QSignalSpy spy(&m_dm, &DeviceManager::batteryLevelChanged);

    std::cout << ">>> Waiting 30 seconds for a battery notification...\n"
              << "    (Move the mouse occasionally to keep it awake)\n";

    QEventLoop loop;
    QObject::connect(&m_dm, &DeviceManager::batteryLevelChanged, &loop, &QEventLoop::quit);
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);
    loop.exec();

    if (spy.count() > 0) {
        std::cout << "Battery notification received: " << m_dm.batteryLevel()
                  << "% charging=" << m_dm.batteryCharging() << "\n";
        EXPECT_GE(m_dm.batteryLevel(), 0);
        EXPECT_LE(m_dm.batteryLevel(), 100);
    } else {
        std::cout << "No battery notification in 30 seconds\n";
        GTEST_SKIP() << "No battery notification received (device-dependent)";
    }
}

TEST_F(HardwareFixture, RapidDisconnectReconnectStability) {
    std::cout << ">>> Turn mouse off and on 3 times rapidly (5 seconds between each).\n"
              << "    You have 30 seconds total...\n";

    int disconnects = 0;
    int reconnects = 0;

    QObject::connect(&m_dm, &DeviceManager::deviceDisconnected, [&]() {
        disconnects++;
        std::cout << "  Disconnect #" << disconnects << "\n";
    });
    QObject::connect(&m_dm, &DeviceManager::deviceSetupComplete, [&]() {
        reconnects++;
        std::cout << "  Reconnect #" << reconnects << ": " << m_dm.deviceName().toStdString() << "\n";
    });

    QEventLoop loop;
    QTimer::singleShot(30000, &loop, &QEventLoop::quit);
    loop.exec();

    std::cout << "Total: " << disconnects << " disconnects, " << reconnects << " reconnects\n";

    if (disconnects == 0) {
        GTEST_SKIP() << "No disconnects detected";
    }

    if (m_dm.deviceConnected()) {
        EXPECT_FALSE(m_dm.deviceName().isEmpty());
        EXPECT_GE(m_dm.batteryLevel(), 0);
        std::cout << "Device stable after " << disconnects << " cycles\n";
    } else {
        QSignalSpy spy(&m_dm, &DeviceManager::deviceSetupComplete);
        if (spy.wait(5000)) {
            EXPECT_TRUE(m_dm.deviceConnected());
            std::cout << "Device reconnected after final cycle\n";
        } else {
            FAIL() << "Device did not reconnect after rapid cycling";
        }
    }
}
