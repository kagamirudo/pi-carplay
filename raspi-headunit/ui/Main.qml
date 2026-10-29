import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: window
    width: 480
    height: 320
    visible: true
    visibility: Window.FullScreen
    title: "Headunit"
    color: "#121612"
    font.pixelSize: 14

    palette.window: "#121612"
    palette.windowText: "#f4f6f2"
    palette.button: "#243024"
    palette.buttonText: "#f4f6f2"
    palette.highlight: "#e6b325"
    palette.highlightedText: "#1a1404"
    palette.base: "#1a221a"
    palette.text: "#f4f6f2"

    header: Rectangle {
        height: 42
        color: "#0c100c"
        Row {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4
            Repeater {
                model: [
                    { label: "Home", page: 0 },
                    { label: "Phone", page: 1 },
                    { label: "Cam", page: 2 },
                    { label: "BT", page: 3 },
                    { label: "Setup", page: 4 }
                ]
                delegate: Button {
                    required property var modelData
                    text: modelData.label
                    width: (parent.width - 16) / 5
                    height: 34
                    checkable: true
                    checked: stack.currentIndex === modelData.page
                    onClicked: stack.currentIndex = modelData.page
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? "#1a1404" : "#f4f6f2"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                        font.bold: parent.checked
                    }
                    background: Rectangle {
                        radius: 4
                        color: parent.checked ? "#e6b325" : (parent.down ? "#314231" : "#243024")
                    }
                }
            }
        }
    }

    StackLayout {
        id: stack
        anchors.fill: parent
        currentIndex: 0
        HomePage {}
        ConnectionPage {}
        DashcamPage {}
        BluetoothPage {}
        SettingsPage {}
    }
}
