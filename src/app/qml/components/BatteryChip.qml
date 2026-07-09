import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Logitune

// Options+-style battery indicator: percentage, vertical battery outline,
// and connection type icon (Bolt receiver or Bluetooth).
Rectangle {
    id: chip
    width: chipRow.implicitWidth + 16
    height: 28
    radius: 4
    color: Theme.cardBg
    border.color: Theme.accent
    border.width: 1
    visible: chip.level > 0

    readonly property int level: DeviceModel.batteryLevel
    readonly property bool charging: DeviceModel.batteryCharging
    readonly property string connType: DeviceModel.connectionType

    readonly property color battColor: {
        if (charging) return Theme.accent;
        if (level <= 15) return "#FF4444";
        if (level <= 25) return Theme.batteryWarning;
        return Theme.accent;
    }

    RowLayout {
        id: chipRow
        anchors.centerIn: parent
        spacing: 6

        // Percentage text
        Text {
            text: chip.level > 0 ? chip.level + "%" : ""
            font.pixelSize: 12
            font.bold: true
            color: chip.battColor
        }

        // Vertical battery outline with fill level
        Item {
            width: 12; height: 20

            // Battery tip (positive terminal, top)
            Rectangle {
                x: 3; y: 0
                width: 6; height: 2
                radius: 1
                color: chip.battColor
            }

            // Battery body outline
            Rectangle {
                x: 0; y: 2
                width: 12; height: 18
                radius: 2
                color: "transparent"
                border.color: chip.battColor
                border.width: 1.5

                // Fill bar inside (fills from bottom up)
                Rectangle {
                    x: 2
                    width: 8
                    radius: 1
                    color: chip.battColor
                    y: 2 + (1 - chip.level / 100) * 14
                    height: Math.max(0, (chip.level / 100) * 14)
                }

                // Charging bolt overlay inside the battery
                // Bolt in battColor (on empty area) + cardBg (on filled area)
                // Use two layers clipped by the fill level
                Item {
                    anchors.centerIn: parent
                    width: 8; height: 12
                    visible: chip.charging
                    clip: true

                    // Bolt in battColor (visible on empty/unfilled background)
                    Image {
                        width: parent.width; height: parent.height
                        sourceSize: Qt.size(32, 48)
                        smooth: true; mipmap: true
                        source: "data:image/svg+xml," + encodeURIComponent(
                            '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 8 12">'
                            + '<polygon points="5,0 1,6 4,6 3,12 7,5 4,5" fill="' + chip.battColor + '"/>'
                            + '</svg>')
                    }

                    // Bolt in cardBg, clipped to only show over the filled area
                    Item {
                        width: parent.width; height: parent.height
                        clip: true
                        y: parent.height * (1 - chip.level / 100)

                        Image {
                            width: parent.parent.width; height: parent.parent.height
                            y: -parent.y
                            sourceSize: Qt.size(32, 48)
                            smooth: true; mipmap: true
                            source: "data:image/svg+xml," + encodeURIComponent(
                                '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 8 12">'
                                + '<polygon points="5,0 1,6 4,6 3,12 7,5 4,5" fill="' + Theme.cardBg + '"/>'
                                + '</svg>')
                        }
                    }
                }
            }
        }

        // Logi Bolt receiver icon — hexagon with bolt cutout (3 polygons)
        Item {
            width: 18; height: 18
            visible: chip.connType === "Bolt"

            Image {
                anchors.fill: parent
                sourceSize: Qt.size(72, 72)
                smooth: true
                mipmap: true
                source: "data:image/svg+xml," + encodeURIComponent(
                    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">'
                    + '<polygon points="25,6 75,6 97,50 75,94 25,94 3,50" fill="' + chip.battColor + '"/>'
                    + '<polygon points="57,0 30,44 44,44 71,0" fill="' + Theme.cardBg + '"/>'
                    + '<polygon points="30,44 30,56 70,56 70,44" fill="' + Theme.cardBg + '"/>'
                    + '<polygon points="56,56 29,100 43,100 70,56" fill="' + Theme.cardBg + '"/>'
                    + '</svg>')
            }
        }

        // Bluetooth icon
        Item {
            width: 16; height: 16
            visible: chip.connType === "Bluetooth"

            Image {
                anchors.fill: parent
                sourceSize: Qt.size(64, 64)
                smooth: true
                mipmap: true
                source: "data:image/svg+xml," + encodeURIComponent(
                    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">'
                    + '<path d="M12 2L18 7L12 12L18 17L12 22" fill="none" stroke="' + chip.battColor + '" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>'
                    + '<line x1="12" y1="2" x2="12" y2="22" stroke="' + chip.battColor + '" stroke-width="2" stroke-linecap="round"/>'
                    + '<line x1="6" y1="7" x2="12" y2="12" stroke="' + chip.battColor + '" stroke-width="2" stroke-linecap="round"/>'
                    + '<line x1="6" y1="17" x2="12" y2="12" stroke="' + chip.battColor + '" stroke-width="2" stroke-linecap="round"/>'
                    + '</svg>')
            }
        }
    }
}
