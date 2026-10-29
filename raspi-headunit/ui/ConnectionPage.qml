import QtQuick
import QtQuick.Controls

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
        spacing: 4

        Label { text: shell.androidAuto; color: "#f4f6f2"; font.pixelSize: 14; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: shell.carPlay; color: "#f4f6f2"; font.pixelSize: 14; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: shell.screenOwner; color: "#e6b325"; font.pixelSize: 14; width: parent.width }
        Label { text: shell.videoSummary; color: "#9aa394"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }

        Rectangle { width: parent.width; height: 1; color: "#2c382c" }

        Label {
            text: "Link: " + shell.linkState
            color: "#f4f6f2"
            font.pixelSize: 15
            font.bold: true
        }
        Label {
            text: shell.linkDetail
            color: "#f4f6f2"
            font.pixelSize: 13
            width: parent.width
            wrapMode: Text.WordWrap
        }
        ActionButton {
            width: parent.width
            text: "Look for phone"
            onClicked: shell.lookForPhone()
        }

        Label { text: "USB  " + shell.usbSummary; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: "Wi-Fi  " + shell.wifiSummary; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: "BT  " + shell.bluetoothSummary; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: "Audio  " + shell.audioSummary; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
    }
}
