#pragma once
#include "hidpp/HidppTypes.h"
#include <cstdint>

namespace logitune::hidpp::features {

// UnifiedBattery (0x1004) status enum — from Solaar
enum class BatteryState : uint8_t {
    Discharging    = 0,
    Recharging     = 1,
    AlmostFull     = 2,
    Full           = 3,
    SlowRecharge   = 4,
    InvalidBattery = 5,
    ThermalError   = 6,
};

struct BatteryStatus {
    int level;            // 0-100 percentage (params[0])
    uint8_t levelBitmask; // fallback level if percentage is 0 (params[1])
    BatteryState state;   // charging state (params[2])
    bool charging;        // derived: state is 1, 2, 3, or 4
};

class Battery {
public:
    // UnifiedBattery (0x1004): params[0]=%, params[1]=level bitmask, params[2]=status
    static BatteryStatus parseStatus(const Report &r);

    // BatteryStatus (0x1000) legacy: params[0]=%, params[1]=next threshold, params[2]=status
    static BatteryStatus parseStatusLegacy(const Report &r);

    static constexpr uint8_t kFnGetCapabilities = 0x00;
    static constexpr uint8_t kFnGetStatus = 0x01;
};

} // namespace logitune::hidpp::features
