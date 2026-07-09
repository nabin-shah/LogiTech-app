import QtQuick
import QtTest
import Logitune

Item {
    width: 200; height: 100

    LogituneToggle {
        id: toggle
    }

    TestCase {
        name: "LogituneToggle"
        when: windowShown

        Component {
            id: signalSpyComponent
            SignalSpy {}
        }

        function init() {
            toggle.checked = false
        }

        function test_defaultUnchecked() {
            compare(toggle.checked, false)
        }

        function test_clickTogglesOn() {
            var spy = createTemporaryObject(signalSpyComponent, this, { target: toggle, signalName: "toggled" })
            waitForRendering(toggle)

            mouseClick(toggle, 14, 8)
            compare(toggle.checked, true, "click should toggle checked to true")
            compare(spy.count, 1, "toggled signal should fire once")
            compare(spy.signalArguments[0][0], true)
        }

        function test_clickTogglesBackOff() {
            toggle.checked = true
            var spy = createTemporaryObject(signalSpyComponent, this, { target: toggle, signalName: "toggled" })
            waitForRendering(toggle)

            mouseClick(toggle, 14, 8)
            compare(toggle.checked, false, "click should toggle checked to false")
            compare(spy.count, 1)
        }

        function test_doubleToggleRestoresState() {
            waitForRendering(toggle)
            mouseClick(toggle, 14, 8)
            compare(toggle.checked, true)
            mouseClick(toggle, 14, 8)
            compare(toggle.checked, false, "double click should restore original state")
        }
    }
}
