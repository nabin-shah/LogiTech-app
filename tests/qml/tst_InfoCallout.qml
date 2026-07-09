import QtQuick
import QtTest
import Logitune

Item {
    width: 300; height: 120

    InfoCallout {
        id: callout
        width: 200; height: 80
        title: "Scroll wheel"
        settings: ["Smooth scrolling: On", "SmartShift: Off"]
        calloutType: "scrollwheel"
    }

    TestCase {
        name: "InfoCallout"
        when: windowShown

        Component {
            id: signalSpyComponent
            SignalSpy {}
        }

        function test_displaysTitle() {
            compare(callout.title, "Scroll wheel")
            compare(callout.calloutType, "scrollwheel")
        }

        function test_clickEmitsCalloutType() {
            var spy = createTemporaryObject(signalSpyComponent, this, { target: callout, signalName: "calloutClicked" })
            waitForRendering(callout)

            mouseClick(callout, 100, 40)
            compare(spy.count, 1, "clicking the callout should emit calloutClicked()")
            compare(spy.signalArguments[0][0], "scrollwheel")
        }
    }
}
