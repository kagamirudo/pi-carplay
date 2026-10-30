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

        // Status header: state + the key record geometry on one line.
        Rectangle {
            width: parent.width
            height: header.height + 16
            radius: 6
            color: "#1a221a"
            border.color: shell.recordState.toLowerCase() === "recording" ? "#e6b325" : "#314231"
            Column {
                id: header
                x: 10; y: 8
                width: parent.width - 20
                spacing: 2
                Label {
                    text: shell.recordState.toUpperCase()
                    color: shell.recordState.toLowerCase() === "recording" ? "#e6b325" : "#f4f6f2"
                    font.pixelSize: 26
                    font.bold: true
                }
                Label { text: shell.recordDetail; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
            }
        }

        // Primary controls, side by side.
        RowLayout {
            width: parent.width
            spacing: 8
            ActionButton {
                text: shell.recordState.toLowerCase() === "recording" ? "Stop" : "Record"
                Layout.fillWidth: true
                onClicked: shell.requestRecord()
            }
            ActionButton {
                text: shell.previewOpen ? "Close preview" : "Preview"
                Layout.fillWidth: true
                onClicked: shell.togglePreview()
            }
        }

        // Details grouped in one card.
        Rectangle {
            width: parent.width
            height: details.height + 16
            radius: 6
            color: "#141a14"
            Column {
                id: details
                x: 10; y: 8
                width: parent.width - 20
                spacing: 3
                Label { text: shell.recordGeometry; color: "#d5ddd2"; font.pixelSize: 13 }
                Label { text: shell.segmentLabel; color: "#d5ddd2"; font.pixelSize: 13 }
                Label { text: shell.focusSummary; color: "#9aa394"; font.pixelSize: 12; width: parent.width; wrapMode: Text.WordWrap }
                Label { text: "Last clip: " + shell.lastClip; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
            }
        }

        // Storage card.
        Rectangle {
            width: parent.width
            height: storage.height + 16
            radius: 6
            color: "#141a14"
            Column {
                id: storage
                x: 10; y: 8
                width: parent.width - 20
                spacing: 3
                Label { text: shell.storageSummary; color: "#f4f6f2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
                Label { text: "Loop limit " + shell.storageLimitLabel; color: "#9aa394"; font.pixelSize: 12 }
            }
        }
    }
}
