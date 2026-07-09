#pragma once
#include "hidpp/HidppTypes.h"
#include <cstdint>
#include <vector>

namespace logitune::hidpp::features {

struct GestureEvent {
    int dx;         // horizontal displacement delta
    int dy;         // vertical displacement delta
    bool released;  // true if thumb button was released (end of gesture)
};

class GestureV2 {
public:
    static GestureEvent parseGestureEvent(const Report &r);
    static std::vector<uint8_t> buildSetGestureEnable(bool enable);

    static constexpr uint8_t kFnSetGestureEnable = 0x05;
};

} // namespace logitune::hidpp::features
