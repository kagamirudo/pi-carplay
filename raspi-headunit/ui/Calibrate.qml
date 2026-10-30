import QtQuick
import QtQuick.Window

// Touch calibration (headunit --calibrate-touch). Taps are read from evdev by
// the calibrator, not from this window, so nothing here handles input.
Window {
    id: window
    width: 480
    height: 320
    visible: true
    visibility: Window.FullScreen
    title: "Touch calibration"
    color: "#000000"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    Item {
        id: cross
        objectName: "calibrationCross"
        visible: !calibrator.finished
        x: calibrator.target.x - width / 2
        y: calibrator.target.y - height / 2
        width: 40
        height: 40

        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: 2
            color: "#e6b325"
        }
        Rectangle {
            anchors.centerIn: parent
            width: 2
            height: parent.height
            color: "#e6b325"
        }
        Rectangle {
            anchors.centerIn: parent
            width: 10
            height: 10
            radius: 5
            color: "transparent"
            border.color: "#f4f6f2"
            border.width: 2
        }
    }

    Column {
        objectName: "calibrationText"
        anchors.horizontalCenter: parent.horizontalCenter
        // Keep the text clear of the cross: top half when the cross is low, and the other way round.
        y: calibrator.target.y > window.height / 2 || calibrator.finished ? window.height * 0.25 : window.height * 0.6
        spacing: 4

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: calibrator.message
            color: "#f4f6f2"
            font.pixelSize: 18
            font.bold: true
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !calibrator.finished
            text: "Target " + (calibrator.step + 1) + " of " + calibrator.steps + " · " + calibrator.secondsLeft + " s"
            color: "#a8b0a4"
            font.pixelSize: 14
        }
    }
}
