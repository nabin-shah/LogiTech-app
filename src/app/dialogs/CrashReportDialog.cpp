#include "dialogs/CrashReportDialog.h"
#include "dialogs/GitHubIssueBuilder.h"
#include "logging/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QDesktopServices>
#include <QProcess>
#include <QClipboard>
#include <QApplication>

namespace logitune {

CrashReportDialog::CrashReportDialog(Mode mode, const CrashInfo &info, QWidget *parent)
    : QDialog(parent), m_crashInfo(info)
{
    applyDarkStyle();
    setupUi(mode, info);
}

void CrashReportDialog::setDeviceInfo(const QString &name, const QString &serial)
{
    m_deviceName = name;
    m_deviceSerial = serial;
}

void CrashReportDialog::applyDarkStyle()
{
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #1a1a1a; }"
        "QLabel { color: #ffffff; }"
        "QLineEdit { background-color: #2a2a2a; color: #ffffff; border: 1px solid #444; "
        "  border-radius: 6px; padding: 8px; font-size: 13px; }"
        "QTextEdit { background-color: #111111; color: #cccccc; border: 1px solid #333; "
        "  border-radius: 6px; padding: 8px; font-family: monospace; font-size: 11px; }"
        "QPushButton { background-color: #00EAD0; color: #000000; border: none; "
        "  border-radius: 6px; padding: 10px 24px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #33FFDD; }"
        "QPushButton#closeBtn { background-color: transparent; color: #888888; "
        "  border: 1px solid #444; }"
        "QPushButton#closeBtn:hover { color: #ffffff; border-color: #666; }"
    ));
}

void CrashReportDialog::setupUi(Mode mode, const CrashInfo &info)
{
    setWindowTitle(QStringLiteral("Logitune — Report Bug"));
    setMinimumSize(520, 400);
    resize(580, 500);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(24, 24, 24, 24);

    // Title
    auto *title = new QLabel(this);
    switch (mode) {
    case CaughtCrash:
        title->setText(QStringLiteral("Logitune crashed unexpectedly"));
        break;
    case RecoveryReport:
        title->setText(QStringLiteral("Logitune didn't shut down cleanly last time"));
        break;
    case ManualReport:
        title->setText(QStringLiteral("Report a bug"));
        break;
    }
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));
    layout->addWidget(title);

    // Crash type badge (if applicable)
    if (!info.type.isEmpty()) {
        auto *badge = new QLabel(info.type, this);
        badge->setStyleSheet(QStringLiteral(
            "background-color: #cc3333; color: white; padding: 4px 12px; "
            "border-radius: 4px; font-size: 12px; font-family: monospace;"));
        badge->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        layout->addWidget(badge);
    }

    // Description field
    auto *descLabel = new QLabel(QStringLiteral("What were you doing?"), this);
    descLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #aaaaaa;"));
    layout->addWidget(descLabel);

    m_descriptionEdit = new QLineEdit(this);
    m_descriptionEdit->setPlaceholderText(QStringLiteral("Describe what happened..."));
    layout->addWidget(m_descriptionEdit);

    // Log viewer
    auto *logLabel = new QLabel(QStringLiteral("Recent log:"), this);
    logLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #888888;"));
    layout->addWidget(logLabel);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);

    QStringList logLines = info.logTail;
    if (logLines.isEmpty())
        logLines = LogManager::instance().tailLog(50);

    // Add stack trace if present
    QString logText;
    if (!info.stackTrace.isEmpty()) {
        logText += QStringLiteral("=== Stack Trace ===\n");
        for (const auto &frame : info.stackTrace)
            logText += frame + "\n";
        logText += QStringLiteral("\n=== Recent Log ===\n");
    }
    for (const auto &line : logLines)
        logText += line + "\n";

    m_logView->setPlainText(logText);
    layout->addWidget(m_logView, 1); // stretch

    // Buttons
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    auto *closeBtn = new QPushButton(QStringLiteral("Close"), this);
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(closeBtn);

    auto *reportBtn = new QPushButton(QStringLiteral("Report Bug"), this);
    connect(reportBtn, &QPushButton::clicked, this, &CrashReportDialog::onReportClicked);
    btnLayout->addWidget(reportBtn);

    layout->addLayout(btnLayout);
}

void CrashReportDialog::onReportClicked()
{
    GitHubIssueBuilder builder;
    builder.setDescription(m_descriptionEdit->text());
    builder.setDeviceName(m_deviceName);
    builder.setDeviceSerial(m_deviceSerial);

    if (!m_crashInfo.type.isEmpty())
        builder.setCrashInfo(m_crashInfo.type, m_crashInfo.stackTrace);

    QStringList logLines = m_crashInfo.logTail;
    if (logLines.isEmpty())
        logLines = LogManager::instance().tailLog(50);
    builder.setLogTail(logLines);

    // Copy log file path to clipboard so the user can attach it to the issue
    QString logPath = LogManager::instance().currentLogPath();
    if (!logPath.isEmpty()) {
        QApplication::clipboard()->setText(logPath);
        qCInfo(lcApp) << "Log file path copied to clipboard:" << logPath;
    }

    QUrl url = builder.buildUrl();
    qCInfo(lcApp) << "Opening bug report URL:" << url.toString().left(200);
    QProcess::startDetached("xdg-open", {url.toString()});
    accept();
}

} // namespace logitune
