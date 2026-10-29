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

        Label {
            text: shell.recordState.toUpperCase()
            color: "#e6b325"
            font.pixelSize: 28
            font.bold: true
        }
        Label { text: shell.recordDetail; color: "#f4f6f2"; font.pixelSize: 14; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: shell.recordGeometry; color: "#d5ddd2"; font.pixelSize: 14 }
        Label { text: shell.segmentLabel; color: "#d5ddd2"; font.pixelSize: 14 }
        Label { text: shell.focusSummary; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: "Last clip: " + shell.lastClip; color: "#d5ddd2"; font.pixelSize: 14 }
        Label { text: shell.storageSummary; color: "#f4f6f2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
        Label {
            text: "Loop limit " + shell.storageLimitLabel
            color: "#9aa394"
            font.pixelSize: 13
        }

        ActionButton {
            width: parent.width
            text: "Record"
            onClicked: shell.requestRecord()
        }
    }
}
