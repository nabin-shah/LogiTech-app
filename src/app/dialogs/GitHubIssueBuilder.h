#pragma once
#include <QString>
#include <QStringList>
#include <QUrl>

namespace logitune {

struct CrashInfo; // forward declare

class GitHubIssueBuilder {
public:
    static constexpr const char *kRepoUrl = "https://github.com/mmaher88/logitune";

    void setDescription(const QString &desc);
    void setDeviceName(const QString &name);
    void setDeviceSerial(const QString &serial);
    void setCrashInfo(const QString &type, const QStringList &stackTrace);
    void setLogTail(const QStringList &lines);

    /// Build the full URL. Truncates log if URL exceeds ~8000 chars.
    QUrl buildUrl() const;

    /// Hash a device serial for privacy: first4***last4
    static QString hashSerial(const QString &serial);

    /// Sanitize paths: replace home dir with ~
    static QString sanitizePath(const QString &path);

private:
    QString m_description;
    QString m_deviceName;
    QString m_serial;
    QString m_crashType;
    QStringList m_stackTrace;
    QStringList m_logTail;
};

} // namespace logitune
