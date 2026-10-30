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

    // A dropdown page selector instead of a row of tabs — scales to any number
    // of pages and stays uncluttered on the 7" screen.
    header: Rectangle {
        height: 48
        color: "#0c100c"
        RowLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 8

            ComboBox {
                id: nav
                objectName: "nav"
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                model: ["Home", "Phone", "Cam", "Bluetooth", "Wi-Fi", "Setup"]
                currentIndex: stack.currentIndex
                onActivated: stack.currentIndex = currentIndex
                font.pixelSize: 16

                contentItem: Text {
                    leftPadding: 12
                    text: nav.displayText
                    color: "#1a1404"
                    font.pixelSize: 16
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle { radius: 5; color: "#e6b325" }
                indicator: Text {
                    x: nav.width - width - 12
                    y: (nav.height - height) / 2
                    text: "▾"
                    color: "#1a1404"
                    font.pixelSize: 16
                }
                delegate: ItemDelegate {
                    width: nav.width
                    required property var modelData
                    required property int index
                    highlighted: nav.highlightedIndex === index
                    contentItem: Text {
                        text: modelData
                        color: "#f4f6f2"
                        font.pixelSize: 16
                        font.bold: index === nav.currentIndex
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 10
                    }
                    background: Rectangle { color: highlighted ? "#314231" : "#1a221a" }
                }
                popup: Popup {
                    y: nav.height + 2
                    width: nav.width
                    implicitHeight: Math.min(contentItem.implicitHeight, 280)
                    padding: 1
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: nav.delegateModel
                        currentIndex: nav.highlightedIndex
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                    background: Rectangle { color: "#0c100c"; border.color: "#314231"; radius: 5 }
                }
            }

            Label {
                text: nav.currentText
                color: "#9aa394"
                font.pixelSize: 14
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
            }
        }
    }

    StackLayout {
        id: stack
        objectName: "stack"
        anchors.fill: parent
        currentIndex: 0
        HomePage {}
        ConnectionPage {}
        DashcamPage {}
        BluetoothPage {}
        WifiPage {}
        SettingsPage {}
    }
}
