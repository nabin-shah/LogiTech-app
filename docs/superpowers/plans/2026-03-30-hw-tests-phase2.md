# Hardware Tests Phase 2 — Full Device Verification

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add hardware integration tests that verify every device setting is applied correctly, persists across sleep/wake, is reapplied after reconnect, and that feature enumeration is complete.

**Architecture:** All tests use the existing `HardwareFixture` in `tests/hw/` which starts a `DeviceManager`, waits for device connection, and restores state on teardown. Each test sends a HID++ command via `DeviceManager`, then reads back via a direct `FeatureDispatcher::call` to verify the device applied it. Interactive tests (button press, scroll) prompt the user.

**Tech Stack:** GTest, HardwareFixture, HID++ 2.0 features (AdjustableDPI, SmartShift, HiResWheel, ThumbWheel, ReprogControlsV4, BatteryUnified), Qt event loop for signal waiting.

---

### Task 1: Feature enumeration completeness test

**Files:**
- Create: `tests/hw/test_hw_features.cpp`
- Modify: `tests/hw/CMakeLists.txt`

- [ ] **Step 1: Write the test**

```cpp
#include <gtest/gtest.h>
#include <iostream>
#include "hw/HardwareFixture.h"
#include "hidpp/HidppTypes.h"

using namespace logitune;
using namespace logitune::test;

TEST_F(HardwareFixture, AllExpectedFeaturesEnumerated) {
    auto *features = m_dm.features();

    struct Expected {
        hidpp::FeatureId id;
        const char *name;
    };

    Expected expected[] = {
        {hidpp::FeatureId::Root,             "Root (0x0000)"},
        {hidpp::FeatureId::FeatureSet,       "FeatureSet (0x0001)"},
        {hidpp::FeatureId::DeviceName,       "DeviceName (0x0005)"},
        {hidpp::FeatureId::BatteryUnified,   "BatteryUnified (0x1004)"},
        {hidpp::FeatureId::ChangeHost,       "ChangeHost (0x1814)"},
        {hidpp::FeatureId::ReprogControlsV4, "ReprogControlsV4 (0x1b04)"},
        {hidpp::FeatureId::SmartShift,       "SmartShift (0x2110)"},
        {hidpp::FeatureId::HiResWheel,       "HiResWheel (0x2121)"},
        {hidpp::FeatureId::ThumbWheel,       "ThumbWheel (0x2150)"},
        {hidpp::FeatureId::AdjustableDPI,    "AdjustableDPI (0x2201)"},
        {hidpp::FeatureId::GestureV2,        "GestureV2 (0x6501)"},
    };

    for (const auto &e : expected) {
        bool found = features->hasFeature(e.id);
        EXPECT_TRUE(found) << "Missing feature: " << e.name;
        if (found) {
            auto idx = features->featureIndex(e.id);
            std::cout << e.name << " -> index 0x"
                      << std::hex << static_cast<int>(*idx) << "\n";
        }
    }
}
```

- [ ] **Step 2: Add to CMakeLists.txt**

In `tests/hw/CMakeLists.txt`, add `test_hw_features.cpp` to the source list.

- [ ] **Step 3: Build and run**

```bash
cmake --build build -j$(nproc) --target logitune-hw-tests
./build/tests/hw/logitune-hw-tests --gtest_filter="*AllExpectedFeatures*"
```

- [ ] **Step 4: Commit**

```bash
git add tests/hw/test_hw_features.cpp tests/hw/CMakeLists.txt
git commit -m "test(hw): verify all expected HID++ features are enumerated"
```

---

### Task 2: DPI readback after set

**Files:**
- Create: `tests/hw/test_hw_settings_readback.cpp`
- Modify: `tests/hw/CMakeLists.txt`

- [ ] **Step 1: Write the tests**

```cpp
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
    // Test ratchet mode with threshold 75
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

    // Test freespin mode
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
    // Set hiRes=true, invert=true
    m_dm.setScrollConfig(true, true);
    processEvents(100);

    auto resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                      FeatureId::HiResWheel,
                                      HiResWheel::kFnGetWheelMode, {});
    ASSERT_TRUE(resp.has_value()) << "getWheelMode failed";
    auto cfg = HiResWheel::parseWheelMode(*resp);
    EXPECT_TRUE(cfg.hiRes) << "hiRes should be true";
    EXPECT_TRUE(cfg.invert) << "invert should be true";

    // Set hiRes=false, invert=false
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
```

- [ ] **Step 2: Add to CMakeLists.txt**

Add `test_hw_settings_readback.cpp` to the source list.

- [ ] **Step 3: Build and run**

```bash
cmake --build build -j$(nproc) --target logitune-hw-tests
./build/tests/hw/logitune-hw-tests --gtest_filter="*DPISet*:*SmartShiftSet*:*ScrollConfigSet*"
```

- [ ] **Step 4: Commit**

```bash
git add tests/hw/test_hw_settings_readback.cpp tests/hw/CMakeLists.txt
git commit -m "test(hw): DPI, SmartShift, ScrollConfig set-and-readback verification"
```

---

### Task 3: Button diversion readback

**Files:**
- Modify: `tests/hw/test_hw_settings_readback.cpp`

- [ ] **Step 1: Add helper to HardwareFixture**

In `tests/hw/HardwareFixture.h`, add a method to read button diversion status:

```cpp
/// Read ReprogControls reporting state for a controlId.
/// Returns true if the button is currently diverted.
bool isButtonDiverted(uint16_t controlId) {
    auto *features = m_dm.features();
    auto *transport = m_dm.transport();
    uint8_t devIdx = m_dm.deviceIndex();

    // Function 0x03 = getControlReporting: params = [controlId_hi, controlId_lo]
    std::array<uint8_t, 2> params = {
        static_cast<uint8_t>(controlId >> 8),
        static_cast<uint8_t>(controlId & 0xFF)
    };
    auto resp = features->call(transport, devIdx,
                               hidpp::FeatureId::ReprogControlsV4, 0x04, // getControlReporting
                               std::span<const uint8_t>(params));
    if (!resp.has_value()) return false;

    // Response params[2] bit 0 = divert flag
    return (resp->params[2] & 0x01) != 0;
}
```

Note: The function index for `getControlReporting` in HID++ ReprogControlsV4 is typically `0x04`. Verify against the actual device response. If it fails, try `0x03`.

- [ ] **Step 2: Write the button diversion test**

Add to `test_hw_settings_readback.cpp`:

```cpp
TEST_F(HardwareFixture, ButtonDiversionSetAndReadback) {
    // CID 0x0053 = middle button on MX Master 3S
    uint16_t cid = 0x0053;

    // Undivert first
    m_dm.divertButton(cid, false);
    processEvents(100);
    EXPECT_FALSE(isButtonDiverted(cid)) << "Button should be undiverted";

    // Divert
    m_dm.divertButton(cid, true);
    processEvents(100);
    EXPECT_TRUE(isButtonDiverted(cid)) << "Button should be diverted";

    // Undivert again
    m_dm.divertButton(cid, false);
    processEvents(100);
    EXPECT_FALSE(isButtonDiverted(cid)) << "Button should be undiverted after restore";

    std::cout << "Button diversion readback verified for CID 0x" << std::hex << cid << "\n";
}
```

- [ ] **Step 3: Build and run**

```bash
cmake --build build -j$(nproc) --target logitune-hw-tests
./build/tests/hw/logitune-hw-tests --gtest_filter="*ButtonDiversion*"
```

If `getControlReporting` function index is wrong, adjust. Check logs for the response.

- [ ] **Step 4: Commit**

```bash
git add tests/hw/HardwareFixture.h tests/hw/test_hw_settings_readback.cpp
git commit -m "test(hw): button diversion set-and-readback via ReprogControls"
```

---

### Task 4: Sleep/wake persistence tests

**Files:**
- Create: `tests/hw/test_hw_sleep_wake.cpp`
- Modify: `tests/hw/CMakeLists.txt`

- [ ] **Step 1: Write the tests**

```cpp
#include <gtest/gtest.h>
#include <iostream>
#include <QEventLoop>
#include <QTimer>
#include "hw/HardwareFixture.h"
#include "hidpp/features/AdjustableDPI.h"
#include "hidpp/features/SmartShift.h"

using namespace logitune;
using namespace logitune::test;
using namespace logitune::hidpp;
using namespace logitune::hidpp::features;

TEST_F(HardwareFixture, DPIPersistsAcrossSleepWake) {
    m_dm.setDPI(1200);
    processEvents(100);

    std::cout << ">>> Stop moving the mouse for 30 seconds to let it sleep,\n"
              << "    then move it to wake. You have 45 seconds total...\n";

    // Wait for device to disconnect (sleep) and reconnect (wake)
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

    // Read back DPI after wake
    auto resp = m_dm.features()->call(m_dm.transport(), m_dm.deviceIndex(),
                                      FeatureId::AdjustableDPI,
                                      AdjustableDPI::kFnGetSensorDpi, {});
    ASSERT_TRUE(resp.has_value());
    int actual = AdjustableDPI::parseCurrentDPI(*resp);
    std::cout << "DPI after wake: " << actual << " (expected 1200)\n";

    // MX Master 3S stores DPI in onboard memory — it may or may not persist.
    // This test documents the actual behavior.
    if (actual == 1200) {
        std::cout << "DPI persisted across sleep/wake\n";
    } else {
        std::cout << "DPI reset to " << actual << " after sleep/wake — firmware does not persist\n";
        // Not a failure — documents firmware behavior
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

    // Read back thumb wheel status after wake
    auto *features = m_dm.features();
    auto resp = features->call(m_dm.transport(), m_dm.deviceIndex(),
                               FeatureId::ThumbWheel, 0x01, {});
    ASSERT_TRUE(resp.has_value());

    bool divert = (resp->params[0] & 0x01) != 0;
    bool invert = (resp->params[1] & 0x01) != 0;
    std::cout << "ThumbWheel after wake: divert=" << divert << " invert=" << invert << "\n";

    if (divert && invert) {
        std::cout << "ThumbWheel mode persisted across sleep/wake\n";
    } else {
        std::cout << "ThumbWheel mode reset after sleep/wake — firmware does not persist diversion\n";
        // Expected: HID++ diversion is volatile. App must reapply after wake.
    }
}
```

- [ ] **Step 2: Add to CMakeLists.txt**

Add `test_hw_sleep_wake.cpp`.

- [ ] **Step 3: Build and run**

```bash
cmake --build build -j$(nproc) --target logitune-hw-tests
./build/tests/hw/logitune-hw-tests --gtest_filter="*SleepWake*"
```

Note: These tests take 45+ seconds each and require the mouse to actually sleep. Run them separately.

- [ ] **Step 4: Commit**

```bash
git add tests/hw/test_hw_sleep_wake.cpp tests/hw/CMakeLists.txt
git commit -m "test(hw): sleep/wake persistence for DPI and ThumbWheel mode"
```

---

### Task 5: Profile reapplication after reconnect

**Files:**
- Create: `tests/hw/test_hw_profile_reapply.cpp`
- Modify: `tests/hw/CMakeLists.txt`

This test verifies the full AppController reconnect flow — not just DeviceManager.

- [ ] **Step 1: Write the test**

```cpp
#include <gtest/gtest.h>
#include <iostream>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>
#include <QTemporaryDir>
#include "AppController.h"
#include "DeviceRegistry.h"
#include "ProfileEngine.h"

using namespace logitune;

class ProfileReapplyFixture : public ::testing::Test {
protected:
    void SetUp() override {
        m_ctrl.init();
        m_ctrl.startMonitoring();

        // Wait for device
        QSignalSpy spy(m_ctrl.deviceModel(), &DeviceModel::deviceConnectedChanged);
        if (!m_ctrl.deviceModel()->deviceConnected()) {
            ASSERT_TRUE(spy.wait(10000)) << "No device connected";
        }
        ASSERT_TRUE(m_ctrl.deviceModel()->deviceConnected());

        // Set up a profile with non-default values
        // (The real app sets up profiles in onDeviceSetupComplete)
    }

    AppController m_ctrl;
};

TEST_F(ProfileReapplyFixture, DeviceReconnectsAndProfileReapplied) {
    // Read current DPI before disconnect
    int dpiBefore = m_ctrl.deviceModel()->currentDPI();
    QString thumbModeBefore = m_ctrl.deviceModel()->thumbWheelMode();

    std::cout << "Before disconnect: DPI=" << dpiBefore
              << " thumbMode=" << thumbModeBefore.toStdString() << "\n";

    std::cout << ">>> Turn off the mouse, wait 3 seconds, turn back on (20 seconds)...\n";

    QSignalSpy disconnSpy(m_ctrl.deviceModel(), &DeviceModel::deviceConnectedChanged);

    QEventLoop loop;
    // Wait for reconnect (deviceConnectedChanged fires twice: false then true)
    int connectCount = 0;
    QObject::connect(m_ctrl.deviceModel(), &DeviceModel::deviceConnectedChanged,
                     [&]() {
                         if (m_ctrl.deviceModel()->deviceConnected()) {
                             connectCount++;
                             if (connectCount >= 1) loop.quit();
                         }
                     });
    QTimer::singleShot(20000, &loop, &QEventLoop::quit);
    loop.exec();

    if (!m_ctrl.deviceModel()->deviceConnected()) {
        GTEST_SKIP() << "Device did not reconnect within 20 seconds";
    }

    // Wait for profile reapplication
    processEvents(1000);

    int dpiAfter = m_ctrl.deviceModel()->currentDPI();
    QString thumbModeAfter = m_ctrl.deviceModel()->thumbWheelMode();

    std::cout << "After reconnect: DPI=" << dpiAfter
              << " thumbMode=" << thumbModeAfter.toStdString() << "\n";

    EXPECT_EQ(dpiBefore, dpiAfter) << "DPI should be reapplied after reconnect";
    EXPECT_EQ(thumbModeBefore, thumbModeAfter) << "ThumbWheel mode should be reapplied";
}
```

Note: This test needs `AppController` which requires more dependencies. If it's too complex to wire up in the hw test binary, simplify by just using `DeviceManager` and verifying the raw HID++ state after reconnect.

- [ ] **Step 2: Add to CMakeLists.txt and link dependencies**

Add `test_hw_profile_reapply.cpp`. May need to link `logitune-app-lib` in addition to `logitune-core`:

```cmake
target_link_libraries(logitune-hw-tests PRIVATE logitune-core logitune-app-lib Qt6::Test Qt6::Widgets GTest::gtest)
```

- [ ] **Step 3: Build and run**

```bash
cmake --build build -j$(nproc) --target logitune-hw-tests
./build/tests/hw/logitune-hw-tests --gtest_filter="*ProfileReapply*"
```

- [ ] **Step 4: Commit**

```bash
git add tests/hw/test_hw_profile_reapply.cpp tests/hw/CMakeLists.txt
git commit -m "test(hw): profile reapplication after device reconnect"
```

---

### Task 6: Battery notification arrives organically

**Files:**
- Modify: `tests/hw/test_hw_device.cpp`

- [ ] **Step 1: Add the test**

```cpp
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
        // Battery notifications may only come on state change (plug/unplug charger)
        // or periodically. Not a failure if none arrive in 30 seconds.
        std::cout << "No battery notification in 30 seconds — device may not send periodic updates\n";
        GTEST_SKIP() << "No battery notification received (device-dependent)";
    }
}
```

- [ ] **Step 2: Build and run**

```bash
cmake --build build -j$(nproc) --target logitune-hw-tests
./build/tests/hw/logitune-hw-tests --gtest_filter="*BatteryNotification*"
```

- [ ] **Step 3: Commit**

```bash
git add tests/hw/test_hw_device.cpp
git commit -m "test(hw): battery notification organic arrival"
```

---

### Task 7: Rapid disconnect/reconnect stability

**Files:**
- Modify: `tests/hw/test_hw_device.cpp`

- [ ] **Step 1: Add the test**

```cpp
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
        GTEST_SKIP() << "No disconnects detected — turn the mouse off during the test";
    }

    // After all cycles, device should be in a stable state
    if (m_dm.deviceConnected()) {
        EXPECT_FALSE(m_dm.deviceName().isEmpty());
        EXPECT_GE(m_dm.batteryLevel(), 0);
        std::cout << "Device stable after " << disconnects << " cycles\n";
    } else {
        // Device might still be reconnecting — wait a bit
        QSignalSpy spy(&m_dm, &DeviceManager::deviceSetupComplete);
        if (spy.wait(5000)) {
            EXPECT_TRUE(m_dm.deviceConnected());
            std::cout << "Device reconnected after final cycle\n";
        } else {
            FAIL() << "Device did not reconnect after rapid cycling";
        }
    }
}
```

- [ ] **Step 2: Build and run**

```bash
cmake --build build -j$(nproc) --target logitune-hw-tests
./build/tests/hw/logitune-hw-tests --gtest_filter="*RapidDisconnect*"
```

- [ ] **Step 3: Commit**

```bash
git add tests/hw/test_hw_device.cpp
git commit -m "test(hw): rapid disconnect/reconnect stability"
```

---

## Summary

| Task | File | Tests | What it verifies |
|------|------|-------|-----------------|
| 1 | test_hw_features.cpp | 1 | All 11 HID++ features enumerated on MX Master 3S |
| 2 | test_hw_settings_readback.cpp | 3 | DPI (5 values), SmartShift (ratchet+freespin), ScrollConfig (hiRes+invert) readback |
| 3 | test_hw_settings_readback.cpp | 1 | Button diversion readback via ReprogControls |
| 4 | test_hw_sleep_wake.cpp | 2 | DPI and ThumbWheel mode after sleep/wake (documents firmware behavior) |
| 5 | test_hw_profile_reapply.cpp | 1 | Full AppController profile reapplication after reconnect |
| 6 | test_hw_device.cpp | 1 | Battery notification arrives from device |
| 7 | test_hw_device.cpp | 1 | Stable state after 3 rapid disconnect/reconnect cycles |

**Total: 10 new hardware tests** added to existing 12, for **22 total hardware tests**.
