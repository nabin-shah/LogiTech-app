#include <gtest/gtest.h>
#include <iostream>
#include "hw/HardwareFixture.h"
#include "hidpp/features/AdjustableDPI.h"
#include "hidpp/features/SmartShift.h"
#include "hidpp/features/HiResWheel.h"

using namespace logitune;
using namespace logitune::test;
using namespace logitune::hidpp;
using namespace logitune::hidpp::features;

TEST_F(HardwareFixture, DPISetAndReadback) {
    int targets[] = {400, 800, 1200, 1800, 3200};
    for (int target : targets) {
        m_dm.setDPI(target);
        processEvents(100);

        auto resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                          FeatureId::AdjustableDPI,
                                          AdjustableDPI::kFnGetSensorDpi, {});
        ASSERT_TRUE(resp.has_value()) << "getSensorDpi failed for DPI=" << target;
        int actual = AdjustableDPI::parseCurrentDPI(*resp);
        EXPECT_NEAR(actual, target, 50) << "DPI mismatch: set " << target << " got " << actual;
        std::cout << "DPI set=" << target << " readback=" << actual << "\n";
    }
}

TEST_F(HardwareFixture, SmartShiftSetAndReadback) {
    m_dm.setSmartShift(true, 75);
    processEvents(100);

    auto resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                      FeatureId::SmartShift,
                                      SmartShift::kFnGetStatus, {});
    ASSERT_TRUE(resp.has_value()) << "SmartShift getStatus failed";
    auto cfg = SmartShift::parseConfig(*resp);
    EXPECT_TRUE(cfg.isRatchet()) << "expected ratchet mode, got mode=" << (int)cfg.mode;
    EXPECT_EQ(cfg.autoDisengage, 75);
    std::cout << "SmartShift: mode=" << (int)cfg.mode << " threshold=" << (int)cfg.autoDisengage << "\n";

    m_dm.setSmartShift(false, 0);
    processEvents(100);

    resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                 FeatureId::SmartShift,
                                 SmartShift::kFnGetStatus, {});
    ASSERT_TRUE(resp.has_value());
    cfg = SmartShift::parseConfig(*resp);
    EXPECT_TRUE(cfg.isFreespin()) << "expected freespin mode, got mode=" << (int)cfg.mode;
}

TEST_F(HardwareFixture, ScrollConfigSetAndReadback) {
    m_dm.setScrollConfig(true, true);
    processEvents(100);

    auto resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                      FeatureId::HiResWheel,
                                      HiResWheel::kFnGetWheelMode, {});
    ASSERT_TRUE(resp.has_value()) << "getWheelMode failed";
    auto cfg = HiResWheel::parseWheelMode(*resp);
    EXPECT_TRUE(cfg.hiRes) << "hiRes should be true";
    EXPECT_TRUE(cfg.invert) << "invert should be true";

    m_dm.setScrollConfig(false, false);
    processEvents(100);

    resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                 FeatureId::HiResWheel,
                                 HiResWheel::kFnGetWheelMode, {});
    ASSERT_TRUE(resp.has_value());
    cfg = HiResWheel::parseWheelMode(*resp);
    EXPECT_FALSE(cfg.hiRes) << "hiRes should be false";
    EXPECT_FALSE(cfg.invert) << "invert should be false";

    std::cout << "ScrollConfig readback verified for both states\n";
}

TEST_F(HardwareFixture, ButtonDiversionSetAndReadback) {
    uint16_t cid = 0x0053; // middle button on MX Master 3S

    m_dm.divertButton(cid, false);
    processEvents(100);
    EXPECT_FALSE(isButtonDiverted(cid)) << "Button should be undiverted";

    m_dm.divertButton(cid, true);
    processEvents(100);
    EXPECT_TRUE(isButtonDiverted(cid)) << "Button should be diverted";

    m_dm.divertButton(cid, false);
    processEvents(100);
    EXPECT_FALSE(isButtonDiverted(cid)) << "Button should be undiverted after restore";

    std::cout << "Button diversion readback verified for CID 0x" << std::hex << cid << "\n";
}
