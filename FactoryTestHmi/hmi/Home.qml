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

    Item {
        id: dragHandle
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 18
        z: 100                     // 确保在所有元素之上

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.ArrowCursor   // 可选，提示可拖动
            property point lastPos: Qt.point(0, 0)

            onPressed: {
                lastPos = Qt.point(mouse.x, mouse.y)
            }
            onPositionChanged: {
                if (pressed) {
                    var delta = Qt.point(mouse.x - lastPos.x, mouse.y - lastPos.y)
                    root.x += delta.x
                    root.y += delta.y
                }
            }
        }
    }

    Rectangle {
        width: 1920
        height: 1080
        color: "#020617"
        radius: 10
        antialiasing: true
    }

    Head {
        id: head
        x: 16
        y: 16
        width: 1888
        height: 96
    }

    UiFactoryTest {
        anchors.top: head.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        width: parent.width - 32
    }
}