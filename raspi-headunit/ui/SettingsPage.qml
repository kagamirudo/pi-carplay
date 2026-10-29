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
        spacing: 6

        Label { text: shell.displaySummary; color: "#f4f6f2"; font.pixelSize: 14; width: parent.width; wrapMode: Text.WordWrap }
        Label { text: shell.touchSummary; color: "#d5ddd2"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }

        ActionButton {
            width: parent.width
            text: "Segment: " + shell.segmentLabel
            onClicked: shell.cycleSegment()
        }
        ActionButton {
            width: parent.width
            text: "Log: " + shell.logLevel
            onClicked: shell.cycleLogLevel()
        }

        RowLayout {
            width: parent.width
            spacing: 6
            ActionButton {
                text: "−"
                Layout.preferredWidth: 48
                onClicked: shell.adjustStorageLimit(-1)
            }
            Label {
                text: "USB limit " + shell.storageLimitLabel
                color: "#f4f6f2"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            ActionButton {
                text: "+"
                Layout.preferredWidth: 48
                onClicked: shell.adjustStorageLimit(1)
            }
        }

        ActionButton {
            width: parent.width
            text: "Save"
            onClicked: shell.saveSettings()
        }
        Label {
            text: shell.saveResult
            color: "#e6b325"
            font.pixelSize: 13
            width: parent.width
            wrapMode: Text.WordWrap
        }
        Label {
            text: "headunit " + shell.version
            color: "#9aa394"
            font.pixelSize: 12
        }
    }
}
