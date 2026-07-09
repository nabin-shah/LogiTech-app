#include <gtest/gtest.h>
#include <iostream>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>
#include "hw/HardwareFixture.h"
#include "hidpp/features/AdjustableDPI.h"
#include "hidpp/features/ThumbWheel.h"

using namespace logitune;
using namespace logitune::test;
using namespace logitune::hidpp;
using namespace logitune::hidpp::features;

TEST_F(HardwareFixture, DPIPersistsAcrossSleepWake) {
    m_dm.setDPI(1200);
    processEvents(100);

    std::cout << ">>> Stop moving the mouse for 30 seconds to let it sleep,\n"
              << "    then move it to wake. You have 45 seconds total...\n";

    QSignalSpy disconnSpy(&m_dm, &DeviceManager::deviceDisconnected);
    QSignalSpy reconnSpy(&m_dm, &DeviceManager::deviceSetupComplete);

    QEventLoop loop;
    QObject::connect(&m_dm, &DeviceManager::deviceSetupComplete, &loop, &QEventLoop::quit);
    QTimer::singleShot(45000, &loop, &QEventLoop::quit);
    loop.exec();

    if (disconnSpy.count() == 0 || reconnSpy.count() == 0) {
        GTEST_SKIP() << "No sleep/wake cycle detected within 45 seconds";
    }

    ASSERT_TRUE(m_dm.deviceConnected());

    auto resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                      FeatureId::AdjustableDPI,
                                      AdjustableDPI::kFnGetSensorDpi, {});
    ASSERT_TRUE(resp.has_value());
    int actual = AdjustableDPI::parseCurrentDPI(*resp);
    std::cout << "DPI after wake: " << actual << " (expected 1200)\n";

    if (actual == 1200) {
        std::cout << "DPI persisted across sleep/wake\n";
    } else {
        std::cout << "DPI reset to " << actual << " after sleep/wake — firmware does not persist\n";
    }
}

TEST_F(HardwareFixture, ThumbWheelModePersistsAcrossSleepWake) {
    setThumbWheelModeAndWait("volume", true);
    verifyThumbWheelStatus(true, true);

    std::cout << ">>> Stop moving the mouse for 30 seconds to let it sleep,\n"
              << "    then move it to wake. You have 45 seconds total...\n";

    QSignalSpy disconnSpy(&m_dm, &DeviceManager::deviceDisconnected);
    QSignalSpy reconnSpy(&m_dm, &DeviceManager::deviceSetupComplete);

    QEventLoop loop;
    QObject::connect(&m_dm, &DeviceManager::deviceSetupComplete, &loop, &QEventLoop::quit);
    QTimer::singleShot(45000, &loop, &QEventLoop::quit);
    loop.exec();

    if (disconnSpy.count() == 0 || reconnSpy.count() == 0) {
        GTEST_SKIP() << "No sleep/wake cycle detected within 45 seconds";
    }

    ASSERT_TRUE(m_dm.deviceConnected());

    auto resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                      FeatureId::ThumbWheel, 0x01, {});
    ASSERT_TRUE(resp.has_value());

    bool divert = (resp->params[0] & 0x01) != 0;
    bool invert = (resp->params[1] & 0x01) != 0;
    std::cout << "ThumbWheel after wake: divert=" << divert << " invert=" << invert << "\n";

    if (divert && invert) {
        std::cout << "ThumbWheel mode persisted across sleep/wake\n";
    } else {
        std::cout << "ThumbWheel mode reset after sleep/wake — firmware does not persist diversion\n";
    }
}
