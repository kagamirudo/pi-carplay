import QtQuick
import QtQuick.Controls

Button {
    id: control
    implicitHeight: 36
    font.pixelSize: 14
    font.bold: true
    contentItem: Text {
        text: control.text
        color: control.enabled ? "#f4f6f2" : "#6d766c"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font: control.font
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 5
        color: !control.enabled ? "#1a1f1a" : (control.down ? "#3d5a32" : "#2f4a2c")
        border.color: "#e6b325"
        border.width: control.down ? 2 : 1
    }
}
