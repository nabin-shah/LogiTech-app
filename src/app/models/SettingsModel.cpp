#include "models/SettingsModel.h"
#include "logging/LogManager.h"
#include "dialogs/CrashReportDialog.h"

namespace logitune {

Q_LOGGING_CATEGORY(lcSettings, "logitune.settings")

SettingsModel::SettingsModel(QObject *parent)
    : QObject(parent)
{
}

bool SettingsModel::loggingEnabled() const
{
    return LogManager::instance().isLoggingEnabled();
}

void SettingsModel::setLoggingEnabled(bool enabled)
{
    if (enabled == loggingEnabled()) return;
    LogManager::instance().setLoggingEnabled(enabled);
    QSettings s;
    s.setValue("logging/enabled", enabled);
    emit loggingEnabledChanged();
}

QString SettingsModel::logFilePath() const
{
    return LogManager::instance().currentLogPath();
}

void SettingsModel::saveThemeDark(bool dark)
{
    QSettings s;
    s.setValue("theme/dark", dark);
    s.sync();
    qCInfo(lcSettings) << "theme/dark saved:" << dark << "to" << s.fileName();
}

void SettingsModel::openBugReport()
{
    qCInfo(lcSettings) << "openBugReport called";
    CrashReportDialog dlg(CrashReportDialog::ManualReport);
    int result = dlg.exec();
    qCInfo(lcSettings) << "CrashReportDialog closed with result:" << result;
}

} // namespace logitune
