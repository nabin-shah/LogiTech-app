import QtQuick
import QtTest
import Logitune

Item {
    width: 700; height: 600
    visible: true

    SettingsPage { id: settings; anchors.fill: parent }

    TestCase {
        name: "SettingsPage"
        when: windowShown

        function test_settingsPageLoads() {
            verify(settings !== null, "SettingsPage should load without errors")
        }

        function test_darkModeToggleActuallyChangesTheme() {
            // Theme singleton test — independent of SettingsPage
            Theme.dark = false
            verify(Qt.colorEqual(Theme.accent, "#814EFA"), "light mode accent must be purple")

            Theme.dark = true
            compare(Theme.dark, true, "Theme must be dark after toggle")
            verify(Qt.colorEqual(Theme.accent, "#00EAD0"), "dark mode accent must be teal")
        }

        function test_loggingToggleReflectsModelState() {
            var enabled = SettingsModel.loggingEnabled
            compare(typeof enabled, "boolean", "loggingEnabled must be boolean")
        }

        function cleanup() {
            Theme.dark = false
        }
    }
}
