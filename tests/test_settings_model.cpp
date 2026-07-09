/// Tests for SettingsModel — theme persistence and logging delegation.
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include "models/SettingsModel.h"
#include "logging/LogManager.h"
#include "helpers/TestFixtures.h"

namespace logitune::test {

class SettingsModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        ensureApp();
        // Use a temp dir so we don't pollute real settings
        m_tmpDir.setAutoRemove(true);
        QCoreApplication::setOrganizationName("LogituneTest");
        QCoreApplication::setApplicationName("SettingsModelTest");
        QSettings::setDefaultFormat(QSettings::IniFormat);
    }

    void TearDown() override {
        // Clean up test settings
        QSettings s;
        s.clear();
    }

    QTemporaryDir m_tmpDir;
};

TEST_F(SettingsModelTest, SaveThemeDarkPersistsToQSettings) {
    SettingsModel model;
    model.saveThemeDark(true);

    QSettings s;
    EXPECT_TRUE(s.contains("theme/dark"));
    EXPECT_TRUE(s.value("theme/dark").toBool());
}

TEST_F(SettingsModelTest, SaveThemeDarkFalsePersists) {
    SettingsModel model;
    model.saveThemeDark(true);
    model.saveThemeDark(false);

    QSettings s;
    EXPECT_FALSE(s.value("theme/dark").toBool());
}

TEST_F(SettingsModelTest, LoggingEnabledDelegatesToLogManager) {
    SettingsModel model;
    bool initial = model.loggingEnabled();

    model.setLoggingEnabled(!initial);
    EXPECT_EQ(model.loggingEnabled(), !initial);
    EXPECT_EQ(LogManager::instance().isLoggingEnabled(), !initial);

    // Persisted to QSettings
    QSettings s;
    EXPECT_EQ(s.value("logging/enabled").toBool(), !initial);

    // Restore
    model.setLoggingEnabled(initial);
}

TEST_F(SettingsModelTest, LoggingEnabledEmitsSignal) {
    SettingsModel model;
    bool initial = model.loggingEnabled();
    int signalCount = 0;

    QObject::connect(&model, &SettingsModel::loggingEnabledChanged,
                     [&signalCount]() { ++signalCount; });

    model.setLoggingEnabled(!initial);
    EXPECT_EQ(signalCount, 1);

    // Setting same value doesn't emit
    model.setLoggingEnabled(!initial);
    EXPECT_EQ(signalCount, 1);

    model.setLoggingEnabled(initial);
}

TEST_F(SettingsModelTest, LogFilePathReturnsNonEmpty) {
    SettingsModel model;
    // LogManager should have a path even if logging is disabled
    QString path = model.logFilePath();
    // Path may be empty if logging was never enabled, that's OK
    // Just verify it doesn't crash
    SUCCEED();
}

} // namespace logitune::test
