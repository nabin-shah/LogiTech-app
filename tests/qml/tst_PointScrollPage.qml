import QtQuick
import QtTest
import Logitune

TestCase {
    name: "PointScrollPage"
    when: windowShown

    Item {
        id: container
        width: 700; height: 600

        PointScrollPage { id: page; anchors.fill: parent }
    }

    function test_pageLoads() {
        verify(page, "PointScrollPage should load without errors")
    }

    function test_noPanelOpenByDefault() {
        compare(page.activePanelType, "")
    }
}
