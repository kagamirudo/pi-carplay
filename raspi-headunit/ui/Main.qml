import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt5Compat.GraphicalEffects

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

    // Shell-only colour adjust (Settings > Display). Two chained effects; each
    // layer is only enabled when non-neutral, so the default path is untouched.
    // layer.effect keeps the wrapped UI fully interactive (unlike a hidden source).
    Item {
        id: fxHue
        anchors.fill: parent
        layer.enabled: shell.displayHue !== 0 || shell.displaySaturation !== 0
        layer.effect: HueSaturation {
            hue: shell.displayHue / 180.0
            saturation: shell.displaySaturation / 100.0
        }

        Item {
            id: fxBright
            anchors.fill: parent
            layer.enabled: shell.displayBrightness !== 0 || shell.displayContrast !== 0
            layer.effect: BrightnessContrast {
                brightness: shell.displayBrightness / 100.0
                contrast: shell.displayContrast / 100.0
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
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
                                id: tab
                                required property var modelData
                                text: modelData.label
                                width: (parent.width - 16) / 5
                                height: 34
                                checked: stack.currentIndex === modelData.page
                                onClicked: stack.currentIndex = modelData.page
                                contentItem: Text {
                                    text: tab.text
                                    color: tab.checked ? "#1a1404" : "#f4f6f2"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 14
                                    font.bold: tab.checked
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: tab.checked ? "#e6b325" : (tab.down ? "#314231" : "#243024")
                                }
                            }
                        }
    readonly property var allPages: ["Home", "Phone", "Cam", "Bluetooth", "Wi-Fi", "Setup"]
    // Car mode hides the Wi-Fi client tab and runs the wireless-Android-Auto hotspot.
    readonly property var navPages: shell.carMode
        ? ["Home", "Phone", "Cam", "Bluetooth", "Setup"]
        : allPages

    Connections {
        target: shell
        function onChanged() {
            if (shell.carMode && stack.currentIndex === 4) {
                stack.currentIndex = 0
            }
            if (wifi.hotspotActive !== shell.carMode) {
                wifi.setHotspot(shell.carMode)
            }
        }
    }

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
                model: window.navPages
                currentIndex: Math.max(0, window.navPages.indexOf(window.allPages[stack.currentIndex]))
                onActivated: stack.currentIndex = window.allPages.indexOf(window.navPages[currentIndex])
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

                StackLayout {
                    id: stack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: 0
                    HomePage {}
                    ConnectionPage {}
                    DashcamPage {}
                    BluetoothPage {}
                    SettingsPage {}
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
