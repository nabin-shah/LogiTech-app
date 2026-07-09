#pragma once
#include <cstdint>
#include <vector>
#include "hidpp/HidppTypes.h"

namespace logitune::hidpp::features {

struct ScrollConfig {
    bool hiRes;     // bit 1 of mode byte
    bool invert;    // bit 2 of mode byte
    bool ratchet;   // from getRatchetSwitch (read-only)
};

class HiResWheel {
public:
    // Parse getWheelMode response (fn1): mode byte in params[0]
    static ScrollConfig parseWheelMode(const Report &r);
    // Parse getRatchetSwitch response (fn3): ratchet in params[0] bit 0
    static bool parseRatchetSwitch(const Report &r);
    // Build setWheelMode params: read-modify-write on mode byte
    static std::vector<uint8_t> buildSetWheelMode(uint8_t currentMode, bool hiRes, bool invert);

    static constexpr uint8_t kFnGetCapabilities   = 0x00;
    static constexpr uint8_t kFnGetWheelMode      = 0x01;
    static constexpr uint8_t kFnSetWheelMode      = 0x02;
    static constexpr uint8_t kFnGetRatchetSwitch   = 0x03;
};

} // namespace logitune::hidpp::features
