#pragma once
#include "logging/CrashHandler.h"
#include <QDialog>

class QTextEdit;
class QLineEdit;

namespace logitune {

class CrashReportDialog : public QDialog {
    Q_OBJECT
public:
    enum Mode { ManualReport, CaughtCrash, RecoveryReport };

    explicit CrashReportDialog(Mode mode, const CrashInfo &info = {},
                                QWidget *parent = nullptr);

    /// Set device info for the GitHub issue body
    void setDeviceInfo(const QString &name, const QString &serial);

private slots:
    void onReportClicked();

private:
    void setupUi(Mode mode, const CrashInfo &info);
    void applyDarkStyle();

    QTextEdit *m_logView = nullptr;
    QLineEdit *m_descriptionEdit = nullptr;
    QString m_deviceName;
    QString m_deviceSerial;
    CrashInfo m_crashInfo;
};

} // namespace logitune
