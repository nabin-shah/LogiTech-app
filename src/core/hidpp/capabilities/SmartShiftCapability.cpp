#include "hidpp/capabilities/SmartShiftCapability.h"

namespace logitune::hidpp::capabilities {

const SmartShiftVariant kSmartShiftVariants[2] = {
    {
        FeatureId::SmartShift,
        features::SmartShift::kFnGetStatus,  // 0x00
        features::SmartShift::kFnSetStatus,  // 0x01
        &features::SmartShift::parseConfig,
        &features::SmartShift::buildSetConfig,
    },
    {
        FeatureId::SmartShiftEnhanced,
        0x01,                                 // Enhanced: fn1 = GetStatus
        0x02,                                 // Enhanced: fn2 = SetStatus
        &features::SmartShift::parseConfig,   // same response layout as V1
        &features::SmartShift::buildSetConfig, // same request layout as V1
    },
};

} // namespace logitune::hidpp::capabilities
