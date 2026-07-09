#pragma once
#include <vector>
#include "hidpp/HidppTypes.h"
#include "hidpp/features/SmartShift.h"

namespace logitune::hidpp::capabilities {

// SmartShift has both a read path (get current config) and a write path
// (set mode + threshold), so the variant carries both function IDs plus
// the request builder.
struct SmartShiftVariant {
    FeatureId feature;
    uint8_t   getFn;
    uint8_t   setFn;
    logitune::hidpp::features::SmartShiftConfig (*parseGet)(const logitune::hidpp::Report&);
    std::vector<uint8_t> (*buildSet)(uint8_t mode, uint8_t autoDisengage);
};

// Known SmartShift variants in preference order.
// V1 (0x2110) is preferred on MX Master 3S and older.
// Enhanced (0x2111) is used on MX Master 4 and newer, with different function IDs.
extern const SmartShiftVariant kSmartShiftVariants[2];

} // namespace logitune::hidpp::capabilities
