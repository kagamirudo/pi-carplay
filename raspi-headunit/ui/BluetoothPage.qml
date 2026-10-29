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
                color: modelData.connected ? "#2f4a2c" : "#1a221a"
                border.color: modelData.connected ? "#e6b325" : "#314231"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    spacing: 6

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            text: modelData.name
                            color: "#f4f6f2"
                            font.pixelSize: 15
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: modelData.connected ? "Connected" : (modelData.paired ? "Paired" : "Nearby")
                            color: modelData.connected ? "#e6b325" : "#9aa394"
                            font.pixelSize: 12
                        }
                    }
                    ActionButton {
                        text: modelData.connected ? "Disconnect" : "Connect"
                        enabled: !bluetooth.busy
                        Layout.preferredWidth: 104
                        onClicked: modelData.connected ? bluetooth.disconnectDevice(modelData.address)
                                                       : bluetooth.connectDevice(modelData.address)
                    }
                    ActionButton {
                        text: "Forget"
                        visible: modelData.paired
                        enabled: !bluetooth.busy
                        Layout.preferredWidth: 72
                        onClicked: bluetooth.forgetDevice(modelData.address)
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
