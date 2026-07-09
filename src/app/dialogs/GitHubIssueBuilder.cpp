#include "dialogs/GitHubIssueBuilder.h"
#include <QSysInfo>
#include <QDir>
#include <QCoreApplication>
#include <QUrlQuery>

namespace logitune {

void GitHubIssueBuilder::setDescription(const QString &desc) { m_description = desc; }
void GitHubIssueBuilder::setDeviceName(const QString &name) { m_deviceName = name; }
void GitHubIssueBuilder::setDeviceSerial(const QString &serial) { m_serial = serial; }

void GitHubIssueBuilder::setCrashInfo(const QString &type, const QStringList &stackTrace)
{
    m_crashType = type;
    m_stackTrace = stackTrace;
}

void GitHubIssueBuilder::setLogTail(const QStringList &lines)
{
    m_logTail = lines;
}

QString GitHubIssueBuilder::hashSerial(const QString &serial)
{
    if (serial.length() <= 8) return serial;
    return serial.left(4) + "***" + serial.right(4);
}

QString GitHubIssueBuilder::sanitizePath(const QString &path)
{
    QString home = QDir::homePath();
    QString result = path;
    result.replace(home, "~");
    return result;
}

QUrl GitHubIssueBuilder::buildUrl() const
{
    // Title
    QString title = m_crashType.isEmpty()
        ? QStringLiteral("Bug: ") + m_description.left(60)
        : QStringLiteral("Crash: ") + m_crashType.left(60);

    // Body
    QString body;
    body += QStringLiteral("## Bug Report\n\n");
    body += QStringLiteral("**Description:**\n%1\n\n").arg(
        m_description.isEmpty() ? QStringLiteral("(no description)") : m_description);

    body += QStringLiteral("**App Version:** %1\n").arg(
        QCoreApplication::applicationVersion().isEmpty()
            ? QStringLiteral("dev") : QCoreApplication::applicationVersion());
    if (!m_deviceName.isEmpty())
        body += QStringLiteral("**Device:** %1 (serial: %2)\n").arg(m_deviceName, hashSerial(m_serial));
    body += QStringLiteral("**OS:** %1 %2\n").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    body += QStringLiteral("**Kernel:** %1\n").arg(QSysInfo::kernelVersion());
    body += QStringLiteral("**Qt:** %1\n\n").arg(QString::fromUtf8(qVersion()));

    // Crash info
    if (!m_crashType.isEmpty()) {
        body += QStringLiteral("**Crash Info:**\n```\n%1\n").arg(m_crashType);
        for (const auto &frame : m_stackTrace)
            body += sanitizePath(frame) + "\n";
        body += QStringLiteral("```\n\n");
    }

    // Log tail
    if (!m_logTail.isEmpty()) {
        body += QStringLiteral("**Log (last %1 lines):**\n```\n").arg(m_logTail.size());
        for (const auto &line : m_logTail)
            body += sanitizePath(line) + "\n";
        body += QStringLiteral("```\n");
    }

    body += QStringLiteral("\n> **Tip:** The full log file path has been copied to your clipboard. "
                          "Drag and drop the file into this issue to attach it.\n");

    // Truncate if URL would be too long
    constexpr int kMaxUrlLength = 7500;
    while (body.toUtf8().size() > kMaxUrlLength && !m_logTail.isEmpty()) {
        // Remove log lines from the front until it fits
        body.clear();
        // Rebuild without log (keep crash info)
        // Simpler: just note the file path
        body += QStringLiteral("## Bug Report\n\n");
        body += QStringLiteral("**Description:**\n%1\n\n").arg(m_description);
        body += QStringLiteral("**App Version:** dev\n");
        if (!m_deviceName.isEmpty())
            body += QStringLiteral("**Device:** %1\n").arg(m_deviceName);
        body += QStringLiteral("**OS:** %1\n**Kernel:** %2\n**Qt:** %3\n\n")
            .arg(QSysInfo::prettyProductName(), QSysInfo::kernelVersion(), QString::fromUtf8(qVersion()));
        if (!m_crashType.isEmpty()) {
            body += QStringLiteral("**Crash Info:**\n```\n%1\n```\n\n").arg(m_crashType);
        }
        body += QStringLiteral("**Logs too large for URL.** Full log at: `~/.local/share/Logitune/logs/`\n");
        break;
    }

    QUrl url(QStringLiteral("%1/issues/new").arg(QLatin1String(kRepoUrl)));
    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("labels", "bug");
    query.addQueryItem("body", body);
    url.setQuery(query);

    return url;
}

} // namespace logitune
