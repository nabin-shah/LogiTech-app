#pragma once
#include "HidppTypes.h"
#include "HidrawDevice.h"

#include <QObject>
#include <atomic>
#include <optional>

namespace logitune::hidpp {

class Transport : public QObject {
    Q_OBJECT
public:
    explicit Transport(HidrawDevice *device, QObject *parent = nullptr);

    std::optional<Report> sendRequest(const Report &request, int timeoutMs = 2000);

    /// Fire-and-forget write — no response wait. Returns true if write succeeded.
    bool sendRequestAsync(const Report &request);

signals:
    void notificationReceived(const Report &report);
    void deviceError(ErrorCode code, uint8_t featureIndex);
    void deviceDisconnected();

private:
    std::optional<Report> trySend(const Report &request, int timeoutMs, int retriesLeft);

    HidrawDevice *m_device;
    std::atomic<bool> m_running{false};
};

} // namespace logitune::hidpp
