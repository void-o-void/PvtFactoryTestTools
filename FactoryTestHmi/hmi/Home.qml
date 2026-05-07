import QtQuick
import QtQuick.Controls

Window {
    id: root
    width: 1920
    height: 1080
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    color:"#00000000"

    x:100
    y:50



    Rectangle {
        width: 1920
        height: 1080
        color: "#020617"
        radius: 10
        antialiasing: true
    }

    Head {
        id: head
        anchors.top: parent.top
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        width: parent.width - 32
        color: "#0f172a"
        height: 96
        border.color: "#1e293b"
        border.width: 1
    }

    UiFactoryTest {
        anchors.top: head.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        width: parent.width - 32
    }

}