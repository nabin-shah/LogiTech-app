#pragma once
#include <cstddef>
#include <optional>
#include "hidpp/FeatureDispatcher.h"

namespace logitune::hidpp::capabilities {

// resolveCapability walks a constexpr table of variants and returns the first
// one whose `feature` is advertised by the given dispatcher. Returns nullopt
// if none of the variants are supported.
//
// Each Variant struct must have a `FeatureId feature;` member. Other fields
// (function IDs, parser pointers) are variant-specific.
template<typename Variant, size_t N>
std::optional<Variant> resolveCapability(FeatureDispatcher* dispatcher,
                                         const Variant (&variants)[N])
{
    if (!dispatcher)
        return std::nullopt;
    for (const auto& v : variants) {
        if (dispatcher->hasFeature(v.feature))
            return v;
    }
    return std::nullopt;
}

} // namespace logitune::hidpp::capabilities
