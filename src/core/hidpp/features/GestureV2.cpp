#include "hidpp/features/GestureV2.h"

namespace logitune::hidpp::features {

GestureEvent GestureV2::parseGestureEvent(const Report &r)
{
    GestureEvent evt;
    evt.dx       = static_cast<int>(static_cast<int16_t>((r.params[0] << 8) | r.params[1]));
    evt.dy       = static_cast<int>(static_cast<int16_t>((r.params[2] << 8) | r.params[3]));
    evt.released = (r.params[4] == 0x01);
    return evt;
}

std::vector<uint8_t> GestureV2::buildSetGestureEnable(bool enable)
{
    return { static_cast<uint8_t>(enable ? 0x01 : 0x00) };
}

} // namespace logitune::hidpp::features
