#pragma once
#include "interfaces/ITransport.h"
#include "hidpp/HidppTypes.h"
#include <QMap>
#include <QVector>
#include <optional>

namespace logitune::test {

class MockTransport : public logitune::ITransport {
    Q_OBJECT
public:
    explicit MockTransport(QObject *parent = nullptr)
        : ITransport(parent)
    {}

    std::optional<hidpp::Report> sendRequest(
        const hidpp::Report &request, int /*timeoutMs*/ = 2000) override
    {
        m_sentRequests.append(request);
        auto it = m_responses.find(request.featureIndex);
        if (it != m_responses.end())
            return it.value();
        return std::nullopt;
    }

    int notificationFd() const override { return -1; }

    std::vector<uint8_t> readRawReport(int /*timeoutMs*/ = 0) override {
        return {};
    }

    // --- Test helpers ---

    void setResponse(uint8_t featureIndex, const hidpp::Report &report) {
        m_responses[featureIndex] = report;
    }

    void simulateNotification(const hidpp::Report &report) {
        emit notificationReceived(report);
    }

    void simulateDisconnect() {
        emit deviceDisconnected();
    }

    const QVector<hidpp::Report> &sentRequests() const { return m_sentRequests; }

    void clearSentRequests() { m_sentRequests.clear(); }

private:
    QMap<uint8_t, hidpp::Report> m_responses;
    QVector<hidpp::Report> m_sentRequests;
};

} // namespace logitune::test
