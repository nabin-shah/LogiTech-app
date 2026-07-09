#pragma once
#include "hidpp/HidppTypes.h"
#include "hidpp/Transport.h"
#include <QObject>
#include <unordered_map>
#include <functional>
#include <vector>
#include <span>

namespace logitune::hidpp {

struct FeatureIdHash {
    std::size_t operator()(FeatureId id) const {
        return std::hash<uint16_t>{}(static_cast<uint16_t>(id));
    }
};

class FeatureDispatcher : public QObject {
    Q_OBJECT
public:
    explicit FeatureDispatcher(QObject *parent = nullptr);

    // Enumerate features from device via Root feature (0x0000)
    // Sends getFeatureID request for each known feature
    bool enumerate(Transport *transport, uint8_t deviceIndex);

    // Manual set for testing (bypasses transport)
    void setFeatureTable(std::vector<std::pair<FeatureId, uint8_t>> table);

    std::optional<uint8_t> featureIndex(FeatureId id) const;
    bool hasFeature(FeatureId id) const;
    void setFeatureIndex(FeatureId id, uint8_t index) { m_featureMap[id] = index; }

    // Send a feature call: resolves FeatureId to index, builds Report, sends via transport
    std::optional<Report> call(Transport *transport, uint8_t deviceIndex,
                               FeatureId feature, uint8_t functionId,
                               std::span<const uint8_t> params = {});

    using ResponseCallback = std::function<void(const Report &)>;

    // Async call with optional completion callback. Returns the softwareId used
    // for matching. The callback is invoked when the response arrives via
    // handleResponse(). If no callback is provided, the response is discarded.
    uint8_t callAsync(Transport *transport, uint8_t deviceIndex,
                      FeatureId feature, uint8_t functionId,
                      std::span<const uint8_t> params = {},
                      ResponseCallback callback = nullptr);

    // Called by DeviceManager when an incoming report has softwareId != 0.
    // Returns true if the report matched a pending callback (consumed).
    bool handleResponse(const Report &report);

private:
    uint8_t nextSoftwareId();

    std::unordered_map<FeatureId, uint8_t, FeatureIdHash> m_featureMap;
    // Pending callbacks keyed by softwareId (1-15)
    std::unordered_map<uint8_t, ResponseCallback> m_pendingCallbacks;
    uint8_t m_nextSwId = 1;  // rotating 1-15
};

} // namespace logitune::hidpp
