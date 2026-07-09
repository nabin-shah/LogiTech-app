import QtQuick
import QtTest
import Logitune

TestCase {
    name: "EasySwitchPage"
    when: windowShown

    Item {
        id: container
        width: 700; height: 600
        visible: true

        EasySwitchPage { id: page; anchors.fill: parent }
    }

    function test_pageLoads() {
        verify(page !== null, "EasySwitchPage should load without errors")
        waitForRendering(container)
        // EasySwitchPage is an Item that should exist in the tree
        compare(page.width, 700)
    }
}
