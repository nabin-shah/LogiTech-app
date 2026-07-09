#pragma once
#include "interfaces/IInputInjector.h"
#include <vector>

namespace logitune {

class UinputInjector : public IInputInjector {
    Q_OBJECT
public:
    explicit UinputInjector(QObject *parent = nullptr);
    ~UinputInjector() override;

    bool init() override;
    void injectKeystroke(const QString &combo) override;
    void injectCtrlScroll(int direction, int steps = 1) override;
    void injectHorizontalScroll(int direction) override;
    void sendDBusCall(const QString &spec) override;
    void launchApp(const QString &command) override;

    void shutdown();

    // Static helpers (testable)
    static std::vector<int> parseKeystroke(const QString &combo);

private:
    int m_uinputFd = -1;
    void emitKey(int keycode, bool press);
    void emitSync();
};

} // namespace logitune
