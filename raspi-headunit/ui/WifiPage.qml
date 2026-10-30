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
                text: wifi.scanning ? "Scanning…" : "Scan"
                enabled: !wifi.scanning && !wifi.busy
                Layout.preferredWidth: 120
                onClicked: wifi.scan()
            }
            Item { Layout.fillWidth: true }
            BusyIndicator {
                running: wifi.scanning || wifi.busy
                visible: running
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
            }
        }

        Label {
            text: wifi.status
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
            model: wifi.networks
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                required property var modelData
                width: list.width
                height: 48
                radius: 5
                readonly property bool active: modelData.active === true
                readonly property bool known: modelData.known === true
                readonly property string ssid: modelData.ssid || ""
                readonly property int signal: modelData.signal || 0
                readonly property string security: modelData.security || ""
                color: active ? "#2f4a2c" : "#1a221a"
                border.color: active ? "#e6b325" : "#314231"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 6
                    spacing: 6

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Label {
                            text: ssid
                            color: "#f4f6f2"
                            font.pixelSize: 15
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            // signal as bars, plus saved/open/secured and the active mark
                            text: (signal >= 67 ? "▂▄▆" : signal >= 34 ? "▂▄" : "▂") + "  "
                                  + (active ? "Connected" : known ? "Saved" : (security === "" ? "Open" : "Not saved"))
                                  + (security === "" ? "" : " · " + security)
                            color: active ? "#e6b325" : "#9aa394"
                            font.pixelSize: 12
                        }
                    }
                    ActionButton {
                        text: "Connect"
                        // Only saved networks can connect (no keyboard to type a new password).
                        visible: known && !active
                        enabled: !wifi.busy
                        Layout.preferredWidth: 100
                        onClicked: wifi.connectNetwork(ssid)
                    }
                    ActionButton {
                        text: "Forget"
                        visible: known
                        enabled: !wifi.busy
                        Layout.preferredWidth: 72
                        onClicked: wifi.forgetNetwork(ssid)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: list.count === 0
                text: "No networks yet. Tap Scan."
                color: "#9aa394"
            }
        }

        Label {
            text: "No keyboard here: join a new network once over SSH (nmcli device wifi connect \"SSID\" password …); it then shows as Saved."
            color: "#6f7a6b"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
