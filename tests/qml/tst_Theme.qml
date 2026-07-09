import QtQuick
import QtTest
import Logitune

TestCase {
    name: "Theme"
    when: windowShown

    function test_darkMode_defaultIsFalse() {
        compare(Theme.dark, false)
    }

    function test_darkMode_accentChanges() {
        Theme.dark = true
        verify(Qt.colorEqual(Theme.accent, "#00EAD0"), "dark accent should be #00EAD0")
        Theme.dark = false
        verify(Qt.colorEqual(Theme.accent, "#814EFA"), "light accent should be #814EFA")
    }

    function test_darkMode_backgroundChanges() {
        Theme.dark = true
        verify(Qt.colorEqual(Theme.background, "#000000"), "dark background should be #000000")
        Theme.dark = false
        verify(Qt.colorEqual(Theme.background, "#FFFFFF"), "light background should be #FFFFFF")
    }

    function test_darkMode_textChanges() {
        Theme.dark = true
        var darkText = Theme.text.toString()
        Theme.dark = false
        var lightText = Theme.text.toString()
        verify(darkText !== lightText, "text color should differ between modes")
    }

    function cleanup() {
        Theme.dark = false
    }
}
