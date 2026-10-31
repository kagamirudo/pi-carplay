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

        Label { text: "Display (shell only)"; color: "#e6b325"; font.pixelSize: 13; font.bold: true; width: parent.width }

        Repeater {
            model: [
                { name: "Brightness", lo: -100, hi: 100, get: function() { return shell.displayBrightness }, set: function(v) { shell.setDisplayBrightness(v) } },
                { name: "Contrast", lo: -100, hi: 100, get: function() { return shell.displayContrast }, set: function(v) { shell.setDisplayContrast(v) } },
                { name: "Saturation", lo: -100, hi: 100, get: function() { return shell.displaySaturation }, set: function(v) { shell.setDisplaySaturation(v) } },
                { name: "Hue", lo: -180, hi: 180, get: function() { return shell.displayHue }, set: function(v) { shell.setDisplayHue(v) } }
            ]
            delegate: RowLayout {
                required property var modelData
                width: column.width
                spacing: 6
                Label {
                    text: modelData.name
                    color: "#f4f6f2"
                    font.pixelSize: 13
                    Layout.preferredWidth: 80
                }
                Slider {
                    Layout.fillWidth: true
                    from: modelData.lo
                    to: modelData.hi
                    stepSize: 1
                    value: modelData.get()
                    onMoved: modelData.set(Math.round(value))
                }
                Label {
                    text: modelData.get()
                    color: "#d5ddd2"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: 34
                }
            }
        }

        ActionButton {
            width: parent.width
            text: "Reset display"
            onClicked: shell.resetDisplayAdjust()
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
