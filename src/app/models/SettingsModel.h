#pragma once
#include <QObject>
#include <QSettings>
#include <QTimer>

namespace logitune {

/// SettingsModel — ViewModel for app-level settings (logging, theme, bug reports).
/// Separates application concerns from DeviceModel's device-specific state.
class SettingsModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool loggingEnabled READ loggingEnabled WRITE setLoggingEnabled NOTIFY loggingEnabledChanged)
    Q_PROPERTY(QString logFilePath READ logFilePath NOTIFY loggingEnabledChanged)

public:
    explicit SettingsModel(QObject *parent = nullptr);

    bool loggingEnabled() const;
    void setLoggingEnabled(bool enabled);
    QString logFilePath() const;

    Q_INVOKABLE void saveThemeDark(bool dark);
    Q_INVOKABLE void openBugReport();
    Q_INVOKABLE void testCrash() {
        QTimer::singleShot(0, [] { throw std::runtime_error("Test crash from UI"); });
    }

signals:
    void loggingEnabledChanged();
};

} // namespace logitune
