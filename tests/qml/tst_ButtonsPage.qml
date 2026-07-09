import QtQuick
import QtTest
import Logitune

TestCase {
    name: "ButtonsPage"
    when: windowShown

    Item {
        id: container
        width: 700; height: 600

        ButtonsPage { id: page; anchors.fill: parent }
    }

    function test_pageLoads() {
        verify(page, "ButtonsPage should load without errors")
    }

    function test_noButtonSelectedByDefault() {
        compare(page.selectedButton, -1)
    }
}
