#pragma once
#include "hidpp/HidppTypes.h"
#include <array>
#include <cstdint>

namespace logitune::hidpp::features {

struct DPISensorInfo {
    int minDPI;
    int maxDPI;
    int stepDPI;
};

class AdjustableDPI {
public:
    // Function 1: getSensorDpiList — returns DPI range (min/max/step)
    static DPISensorInfo parseSensorDpiList(const Report &r);
    // Function 2: getSensorDpi — returns current DPI
    static int parseCurrentDPI(const Report &r);
    // Function 3: setSensorDpi
    static std::array<uint8_t, 3> buildSetDPI(int dpi, uint8_t sensorIndex = 0);

    static constexpr uint8_t kFnGetSensorCount   = 0x00;
    static constexpr uint8_t kFnGetSensorDpiList  = 0x01;
    static constexpr uint8_t kFnGetSensorDpi      = 0x02;
    static constexpr uint8_t kFnSetSensorDpi      = 0x03;
};

} // namespace logitune::hidpp::features
