import QtQuick
import QtQuick.Controls
import QtTest
import Logitune

Item {
    width: 960; height: 640

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: deviceViewComponent
    }

    Component {
        id: deviceViewComponent
        DeviceView {}
    }

    TestCase {
        name: "DeviceView"
        when: windowShown

        function findSideNav(parent) {
            if (!parent || !parent.children) return null
            for (var i = 0; i < parent.children.length; i++) {
                var child = parent.children[i]
                if (child.hasOwnProperty("currentPage") && child.hasOwnProperty("pageSelected"))
                    return child
                var found = findSideNav(child)
                if (found) return found
            }
            return null
        }

        function findProfileBar(parent) {
            if (!parent || !parent.children) return null
            for (var i = 0; i < parent.children.length; i++) {
                var child = parent.children[i]
                if (child.hasOwnProperty("overflowing") && child.hasOwnProperty("scrollOffset"))
                    return child
                var found = findProfileBar(child)
                if (found) return found
            }
            return null
        }

        function init() {
            var view = stack.currentItem
            if (view) {
                var sideNav = findSideNav(view)
                if (sideNav) sideNav.currentPage = "buttons"
            }
        }

        function test_profileBar_visibleOnButtons() {
            var view = stack.currentItem
            verify(view, "DeviceView should be loaded")
            var sideNav = findSideNav(view)
            verify(sideNav, "SideNav should exist")
            var profileBar = findProfileBar(view)
            verify(profileBar, "AppProfileBar should exist")

            compare(sideNav.currentPage, "buttons")
            compare(profileBar.visible, true, "profile bar must be visible on buttons page")
        }

        function test_profileBar_hiddenOnSettings() {
            var view = stack.currentItem
            var sideNav = findSideNav(view)
            var profileBar = findProfileBar(view)
            verify(sideNav); verify(profileBar)

            sideNav.currentPage = "settings"
            sideNav.pageSelected("settings")
            waitForRendering(view)

            compare(profileBar.visible, false, "profile bar must be hidden on settings page")
        }

        function test_profileBar_hiddenOnEasySwitch() {
            var view = stack.currentItem
            var sideNav = findSideNav(view)
            var profileBar = findProfileBar(view)
            verify(sideNav); verify(profileBar)

            sideNav.currentPage = "easyswitch"
            sideNav.pageSelected("easyswitch")
            waitForRendering(view)

            compare(profileBar.visible, false, "profile bar must be hidden on easy-switch page")
        }

        function test_profileBar_visibleOnPointScroll() {
            var view = stack.currentItem
            var sideNav = findSideNav(view)
            var profileBar = findProfileBar(view)
            verify(sideNav); verify(profileBar)

            sideNav.currentPage = "pointscroll"
            sideNav.pageSelected("pointscroll")
            waitForRendering(view)

            compare(profileBar.visible, true, "profile bar must be visible on point & scroll page")
        }

        function test_profileBar_restoresOnBackToButtons() {
            var view = stack.currentItem
            var sideNav = findSideNav(view)
            var profileBar = findProfileBar(view)
            verify(sideNav); verify(profileBar)

            sideNav.currentPage = "settings"
            sideNav.pageSelected("settings")
            waitForRendering(view)
            compare(profileBar.visible, false)

            sideNav.currentPage = "buttons"
            sideNav.pageSelected("buttons")
            waitForRendering(view)
            compare(profileBar.visible, true, "profile bar must reappear when returning to buttons")
        }
    }
}
