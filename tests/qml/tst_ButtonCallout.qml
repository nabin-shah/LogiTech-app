import QtQuick
import QtTest
import Logitune

Item {
    width: 300; height: 100

    ButtonCallout {
        id: callout
        width: 200; height: 60
        actionName: "Forward"
        buttonName: "Button 4"
        selected: false
        showLine: false
    }

    TestCase {
        name: "ButtonCallout"
        when: windowShown

        Component {
            id: signalSpyComponent
            SignalSpy {}
        }

        function init() {
            callout.selected = false
        }

        function test_displaysActionName() {
            compare(callout.actionName, "Forward")
            compare(callout.buttonName, "Button 4")
        }

        function test_clickEmitsSignal() {
            var spy = createTemporaryObject(signalSpyComponent, this, { target: callout, signalName: "clicked" })
            waitForRendering(callout)

            // Card sits at (0,0) with implicit size. Click top-left area of card.
            mouseClick(callout, 20, 20)
            compare(spy.count, 1, "clicking the callout should emit clicked()")
        }

        function test_selectedStateChanges() {
            compare(callout.selected, false)
            callout.selected = true
            compare(callout.selected, true)
        }
    }
}
