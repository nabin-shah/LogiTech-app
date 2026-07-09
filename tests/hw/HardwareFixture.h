#pragma once
#include <gtest/gtest.h>
#include "DeviceManager.h"
#include "DeviceRegistry.h"
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>

namespace logitune::test {

class HardwareFixture : public ::testing::Test {
protected:
    void SetUp() override {
        m_dm.start();

        // Wait for device to connect (up to 10 seconds)
        QSignalSpy spy(&m_dm, &DeviceManager::deviceSetupComplete);
        if (!m_dm.deviceConnected()) {
            ASSERT_TRUE(spy.wait(10000)) << "No device connected — plug in MX Master 3S";
        }
        ASSERT_TRUE(m_dm.deviceConnected());
        ASSERT_NE(m_dm.features(), nullptr);
        ASSERT_NE(m_dm.transport(), nullptr);

        // Record initial state for restoration
        m_initialThumbMode = m_dm.thumbWheelMode();
        m_initialThumbInvert = m_dm.thumbWheelInvert();
        m_initialDPI = m_dm.currentDPI();
        m_initialSmartShift = m_dm.smartShiftEnabled();
        m_initialSmartShiftThreshold = m_dm.smartShiftThreshold();
    }

    void TearDown() override {
        if (m_dm.deviceConnected()) {
            m_dm.setThumbWheelMode(m_initialThumbMode, m_initialThumbInvert);
            m_dm.setDPI(m_initialDPI);
            m_dm.setSmartShift(m_initialSmartShift, m_initialSmartShiftThreshold);
            // Process pending async responses
            processEvents(100);
        }
    }

    /// Process Qt events for the given duration (ms) to let async responses arrive.
    void processEvents(int ms = 50) {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    }

    /// Set thumb wheel mode and wait for the device to confirm via async response.
    void setThumbWheelModeAndWait(const QString &mode, bool invert) {
        m_dm.setThumbWheelMode(mode, invert);
        // Let the event loop process the async response from the device
        processEvents(100);
    }

    /// Read back ThumbWheel status via HID++ getStatus (function 0x01)
    /// and verify it matches expected values.
    void verifyThumbWheelStatus(bool expectDivert, bool expectInvert) {
        auto *features = m_dm.features();
        auto *transport = m_dm.transport();
        uint8_t devIdx = m_dm.deviceIndex();

        auto resp = features->call(transport, devIdx,
                                   hidpp::FeatureId::ThumbWheel, 0x01, {});
        ASSERT_TRUE(resp.has_value()) << "ThumbWheel getStatus failed";

        bool actualDivert = (resp->params[0] & 0x01) != 0;
        bool actualInvert = (resp->params[1] & 0x01) != 0;

        EXPECT_EQ(actualDivert, expectDivert)
            << "divert: expected " << expectDivert << " got " << actualDivert;
        EXPECT_EQ(actualInvert, expectInvert)
            << "invert: expected " << expectInvert << " got " << actualInvert;
    }

    /// Read ReprogControls reporting state for a controlId.
    /// Uses ReprogControlsV4 function 0x04 (getControlReporting).
    /// Note: if 0x04 is absent on a firmware revision, try 0x03
    /// (kFnSetControlReporting); 0x04 is the standard V4 get function.
    bool isButtonDiverted(uint16_t controlId) {
        auto *features = m_dm.features();
        std::array<uint8_t, 2> params = {
            static_cast<uint8_t>(controlId >> 8),
            static_cast<uint8_t>(controlId & 0xFF)
        };
        // ReprogControlsV4 function 0x04 = getControlReporting
        auto resp = features->call(m_dm.transport(), m_dm.deviceIndex(),
                                   hidpp::FeatureId::ReprogControlsV4, 0x04,
                                   std::span<const uint8_t>(params));
        if (!resp.has_value()) return false;
        // Response: params[0-1]=controlId, params[2] bits: 0=divert
        return (resp->params[2] & 0x01) != 0;
    }

    DeviceRegistry m_registry;
    DeviceManager m_dm{&m_registry};

private:
    QString m_initialThumbMode;
    bool m_initialThumbInvert = false;
    int m_initialDPI = 0;
    bool m_initialSmartShift = false;
    int m_initialSmartShiftThreshold = 0;
};

} // namespace logitune::test
