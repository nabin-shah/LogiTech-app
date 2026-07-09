import QtQuick
import QtTest
import Logitune

Item {
    width: 200; height: 50

    BatteryChip { id: chip }

    TestCase {
        name: "BatteryChip"
        when: windowShown

        function test_levelMatchesDeviceModel() {
            compare(chip.level, DeviceModel.batteryLevel,
                    "chip level must match DeviceModel.batteryLevel")
        }

        function test_chargingMatchesDeviceModel() {
            compare(chip.charging, DeviceModel.batteryCharging,
                    "chip charging must match DeviceModel.batteryCharging")
        }
    }
}
