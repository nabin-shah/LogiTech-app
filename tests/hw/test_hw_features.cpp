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
