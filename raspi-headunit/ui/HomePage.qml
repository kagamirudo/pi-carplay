import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
    contentWidth: width
    contentHeight: column.height + 16
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    Column {
        id: column
        x: 8
        y: 8
        width: parent.width - 16
        spacing: 8

        ActionButton {
            width: parent.width
            text: "Android Auto"
            onClicked: shell.openAndroidAuto()
        }

        Rectangle {
            width: parent.width
            height: phoneCol.height + 12
            radius: 6
            color: "#1c241c"
            Column {
                id: phoneCol
                x: 8
                y: 6
                width: parent.width - 16
                spacing: 2
                Label { text: "Phone"; color: "#9aa394"; font.pixelSize: 12 }
                Label {
                    width: parent.width
                    text: shell.homePhone
                    color: "#f4f6f2"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 15
                }
            }
        }

        Rectangle {
            width: parent.width
            height: camCol.height + 12
            radius: 6
            color: "#1c241c"
            Column {
                id: camCol
                x: 8
                y: 6
                width: parent.width - 16
                spacing: 2
                Label { text: "Dash cam"; color: "#9aa394"; font.pixelSize: 12 }
                Label {
                    width: parent.width
                    text: shell.homeCamera
                    color: "#e6b325"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 15
                }
            }
        }

        Rectangle {
            width: parent.width
            height: displayCol.height + 12
            radius: 6
            color: "#1c241c"
            Column {
                id: displayCol
                x: 8
                y: 6
                width: parent.width - 16
                spacing: 2
                Label { text: "Display"; color: "#9aa394"; font.pixelSize: 12 }
                Label {
                    width: parent.width
                    text: shell.homeDisplay
                    color: "#f4f6f2"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 15
                }
            }
        }

        ActionButton {
            width: parent.width
            text: "Refresh"
            onClicked: shell.refresh()
        }
    }
}
