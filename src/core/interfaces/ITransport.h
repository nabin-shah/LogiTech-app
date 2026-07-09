#pragma once
#include "hidpp/HidppTypes.h"
#include <QObject>
#include <optional>
#include <span>

namespace logitune {

class ITransport : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual ~ITransport() = default;

    virtual std::optional<hidpp::Report> sendRequest(
        const hidpp::Report &request, int timeoutMs = 2000) = 0;

    virtual int notificationFd() const = 0;

    virtual std::vector<uint8_t> readRawReport(int timeoutMs = 0) = 0;

signals:
    void notificationReceived(const logitune::hidpp::Report &report);
    void deviceDisconnected();
};

} // namespace logitune
