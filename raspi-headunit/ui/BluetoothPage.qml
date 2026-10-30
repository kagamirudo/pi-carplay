import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            ActionButton {
                text: bluetooth.scanning ? "Scanning…" : "Scan"
                enabled: !bluetooth.scanning
                Layout.preferredWidth: 120
                onClicked: bluetooth.scan()
            }
            ActionButton {
                text: "Refresh"
                Layout.preferredWidth: 100
                onClicked: bluetooth.refresh()
            }
            BusyIndicator {
                running: bluetooth.scanning || bluetooth.busy
                visible: running
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
            }
            Item { Layout.fillWidth: true }
            Label { text: "Car mode"; color: "#f4f6f2"; font.pixelSize: 14 }
            Switch {
                checked: shell.carMode
                onToggled: shell.setCarMode(checked)
            }
        }

        Label {
            text: bluetooth.status
            color: "#e6b325"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: bluetooth.devices
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var modelData
                width: list.width
                height: 48
                radius: 5
                readonly property bool connected: modelData.connected === true
                readonly property bool paired: modelData.paired === true
                readonly property string address: modelData.address || ""
                color: connected ? "#2f4a2c" : "#1a221a"
                border.color: connected ? "#e6b325" : "#314231"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    spacing: 6

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            text: modelData.name || address || "Unknown device"
                            color: "#f4f6f2"
                            font.pixelSize: 15
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: connected ? "Connected" : (paired ? "Paired" : "Nearby")
                            color: connected ? "#e6b325" : "#9aa394"
                            font.pixelSize: 12
                        }
                    }
                    ActionButton {
                        text: connected ? "Disconnect" : "Connect"
                        enabled: !bluetooth.busy && address !== ""
                        Layout.preferredWidth: 104
                        onClicked: connected ? bluetooth.disconnectDevice(address) : bluetooth.connectDevice(address)
                    }
                    ActionButton {
                        text: "Forget"
                        visible: paired
                        enabled: !bluetooth.busy && address !== ""
                        Layout.preferredWidth: 72
                        onClicked: bluetooth.forgetDevice(address)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: list.count === 0
                text: "No devices yet. Tap Scan."
                color: "#9aa394"
            }
        }
    }
}
