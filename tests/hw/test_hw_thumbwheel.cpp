#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QEventLoop>
#include <QTimer>
#include <iostream>
#include "hw/HardwareFixture.h"

using namespace logitune;
using namespace logitune::test;

// =============================================================================
// SetReporting — verify device applies divert/invert via getStatus readback
// =============================================================================

TEST_F(HardwareFixture, SetReportingDivertOnInvertOff) {
    setThumbWheelModeAndWait("volume", false);
    verifyThumbWheelStatus(true, false);
}

TEST_F(HardwareFixture, SetReportingDivertOnInvertOn) {
    setThumbWheelModeAndWait("volume", true);
    verifyThumbWheelStatus(true, true);
}

TEST_F(HardwareFixture, SetReportingDivertOffInvertOff) {
    setThumbWheelModeAndWait("scroll", false);
    verifyThumbWheelStatus(false, false);
}

TEST_F(HardwareFixture, SetReportingDivertOffInvertOn) {
    setThumbWheelModeAndWait("scroll", true);
    verifyThumbWheelStatus(false, true);
}

// =============================================================================
// Diverted mode — verify rotation events arrive
// =============================================================================

TEST_F(HardwareFixture, DivertedModeProducesRotationEvents) {
    setThumbWheelModeAndWait("volume", false);

    QSignalSpy spy(&m_dm, &DeviceManager::thumbWheelRotation);

    std::cout << ">>> Scroll the thumb wheel (5 seconds)...\n";
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_GT(spy.count(), 0) << "No rotation events — is thumb wheel diverted?";
    if (spy.count() > 0)
        std::cout << "Got " << spy.count() << " events, first delta: "
                  << spy.at(0).at(0).toInt() << "\n";
}

// =============================================================================
// Native mode — verify NO rotation events
// =============================================================================

TEST_F(HardwareFixture, NativeModeProducesNoRotationEvents) {
    setThumbWheelModeAndWait("scroll", false);

    QSignalSpy spy(&m_dm, &DeviceManager::thumbWheelRotation);

    std::cout << ">>> Scroll the thumb wheel (5 seconds)...\n";
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    EXPECT_EQ(spy.count(), 0) << "Got rotation events in native mode — still diverted!";
}

// =============================================================================
// Invert flag — verify delta sign flips
// Skip first delta=0 events (initial touch before rotation starts)
// =============================================================================

TEST_F(HardwareFixture, InvertFlagFlipsDeltaSign) {
    // Phase 1: normal (no invert) — collect average sign of clockwise rotation
    setThumbWheelModeAndWait("volume", false);
    QSignalSpy spy1(&m_dm, &DeviceManager::thumbWheelRotation);

    std::cout << ">>> Scroll thumb wheel CLOCKWISE ONLY for 3 seconds...\n"
              << "    (Wait 1 second before starting)\n";
    processEvents(1000);  // give user time to read

    QEventLoop loop1;
    QTimer::singleShot(3000, &loop1, &QEventLoop::quit);
    loop1.exec();

    // Sum all non-zero deltas to get net direction
    int normalSum = 0;
    for (int i = 0; i < spy1.count(); ++i)
        normalSum += spy1.at(i).at(0).toInt();
    ASSERT_NE(normalSum, 0) << "No rotation detected — scroll the wheel";
    std::cout << "Normal sum: " << normalSum << " (" << spy1.count() << " events)\n";

    // Phase 2: inverted — collect average sign of SAME clockwise rotation
    setThumbWheelModeAndWait("volume", true);
    QSignalSpy spy2(&m_dm, &DeviceManager::thumbWheelRotation);

    std::cout << ">>> Scroll thumb wheel CLOCKWISE again for 3 seconds...\n"
              << "    (Wait 1 second before starting)\n";
    processEvents(1000);

    QEventLoop loop2;
    QTimer::singleShot(3000, &loop2, &QEventLoop::quit);
    loop2.exec();

    int invertedSum = 0;
    for (int i = 0; i < spy2.count(); ++i)
        invertedSum += spy2.at(i).at(0).toInt();
    ASSERT_NE(invertedSum, 0) << "No rotation detected — scroll the wheel";
    std::cout << "Inverted sum: " << invertedSum << " (" << spy2.count() << " events)\n";

    // Net sums should have opposite signs
    EXPECT_TRUE((normalSum > 0 && invertedSum < 0) || (normalSum < 0 && invertedSum > 0))
        << "Invert flag did not flip direction: normal=" << normalSum
        << " inverted=" << invertedSum;
}
