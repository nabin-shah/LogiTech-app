#pragma once
#include <QObject>
#include <QString>

namespace logitune {

class IInputInjector : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual ~IInputInjector() = default;

    virtual bool init() = 0;
    virtual void injectKeystroke(const QString &combo) = 0;
    virtual void injectCtrlScroll(int direction, int steps = 1) = 0;
    virtual void injectHorizontalScroll(int direction) = 0;
    virtual void sendDBusCall(const QString &spec) = 0;
    virtual void launchApp(const QString &command) = 0;
};

} // namespace logitune
