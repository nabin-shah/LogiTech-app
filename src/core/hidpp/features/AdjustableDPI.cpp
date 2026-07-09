#include "hidpp/features/AdjustableDPI.h"

namespace logitune::hidpp::features {

static inline int readBE16(const std::array<uint8_t, 16> &params, int offset)
{
    return (static_cast<int>(params[offset]) << 8) | static_cast<int>(params[offset + 1]);
}

DPISensorInfo AdjustableDPI::parseSensorDpiList(const Report &r)
{
    // Function 1 response: [sensorIdx, minDPI_hi, minDPI_lo, ?, step, maxDPI_hi, maxDPI_lo]
    DPISensorInfo info;
    info.minDPI  = readBE16(r.params, 1);  // params[1-2]
    info.stepDPI = static_cast<int>(r.params[4]); // params[4]
    info.maxDPI  = readBE16(r.params, 5);  // params[5-6]
    return info;
}

int AdjustableDPI::parseCurrentDPI(const Report &r)
{
    // Function 2 response: [sensorIdx, dpi_hi, dpi_lo, defaultDpi_hi, defaultDpi_lo]
    return readBE16(r.params, 1);
}

std::array<uint8_t, 3> AdjustableDPI::buildSetDPI(int dpi, uint8_t sensorIndex)
{
    return {
        sensorIndex,
        static_cast<uint8_t>((dpi >> 8) & 0xFF),
        static_cast<uint8_t>(dpi & 0xFF)
    };
}

} // namespace logitune::hidpp::features
