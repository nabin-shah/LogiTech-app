import QtQuick
import QtTest
import Logitune

TestCase {
    name: "Toast"
    when: windowShown

    Item {
        id: container
        width: 400; height: 300

        Toast { id: toast }
    }

    function test_initiallyHidden() {
        compare(toast.opacity, 0)
    }

    function test_showSetsMessage() {
        toast.show("Test notification", 5000)
        tryCompare(toast, "opacity", 1, 1000)
        compare(toast.message, "Test notification")
    }
}
