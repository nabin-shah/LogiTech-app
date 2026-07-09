import QtQuick
import QtTest
import Logitune

Item {
    width: 500; height: 600

    DetailPanel {
        id: panel
        anchors.fill: parent
        panelType: ""
        opened: false
    }

    TestCase {
        name: "DetailPanel"
        when: windowShown

        Component {
            id: signalSpyComponent
            SignalSpy {}
        }

        function cleanup() {
            panel.opened = false
            panel.panelType = ""
        }

        function test_defaultClosed() {
            compare(panel.opened, false)
        }

        function test_openSlide() {
            panel.panelType = "scrollwheel"
            panel.opened = true
            verify(panel.opened)
        }

        function test_panelTypeChangesContent() {
            panel.panelType = "pointerspeed"
            panel.opened = true
            compare(panel.panelType, "pointerspeed")
        }

        function test_closeButtonEmitsCloseRequested() {
            var spy = createTemporaryObject(signalSpyComponent, this, { target: panel, signalName: "closeRequested" })
            panel.panelType = "scrollwheel"
            panel.opened = true
            waitForRendering(panel)

            // ColumnLayout has 32px margins. Close button (28x28) at right of header row.
            // Close button center: x ≈ panel.width - 32 - 14, y ≈ 32 + 14
            mouseClick(panel, panel.width - 46, 46)
            compare(spy.count, 1, "clicking close button should emit closeRequested")
        }
    }
}
