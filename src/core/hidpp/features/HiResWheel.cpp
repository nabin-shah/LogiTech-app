#include "hidpp/features/HiResWheel.h"

namespace logitune::hidpp::features {

ScrollConfig HiResWheel::parseWheelMode(const Report &r)
{
    // Mode byte: bit0=diversion, bit1=hiRes, bit2=invert
    uint8_t mode = r.params[0];
    ScrollConfig cfg;
    cfg.hiRes   = (mode & 0x02) != 0;
    cfg.invert  = (mode & 0x04) != 0;
    cfg.ratchet = false; // filled separately via getRatchetSwitch
    return cfg;
}

bool HiResWheel::parseRatchetSwitch(const Report &r)
{
    return (r.params[0] & 0x01) != 0;
}

std::vector<uint8_t> HiResWheel::buildSetWheelMode(uint8_t currentMode, bool hiRes, bool invert)
{
    // Read-modify-write: preserve bit 0 (diversion), set bits 1+2
    uint8_t newMode = currentMode & 0x01;
    if (hiRes)  newMode |= 0x02;
    if (invert) newMode |= 0x04;
    return { newMode };
}

} // namespace logitune::hidpp::features
