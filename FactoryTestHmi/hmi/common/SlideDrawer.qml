import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    // ===== 公开属性 =====
    property string title: ""
    property real drawerWidth: 460
    property real topOffset: 112    // 从 Head 底部开始（16 + 96）
    default property alias content: contentArea.data

    signal saved()
    signal cancelled()

    // 窗口坐标系
    parent: Overlay.overlay
    x: 0; y: 0
    implicitWidth: 1920
    implicitHeight: 1080

    padding: 0
    closePolicy: Popup.CloseOnEscape
    focus: true
    Overlay.modal: Rectangle { color: "transparent" }

    // 全屏遮罩背景（50% 透明度）
    background: Rectangle {
        id: bgMask
        color: "#80000000"
        opacity: 0
        MouseArea { anchors.fill: parent; onClicked: root.close() }
    }

    // 内容区（抽屉面板 + 动画从右边滑入）
    contentItem: Item {
        implicitWidth: root.implicitWidth
        implicitHeight: root.implicitHeight

        Rectangle {
            id: panel
            x: parent.width
            anchors {
                top: parent.top
                topMargin: root.topOffset
                bottom: parent.bottom
            }
            width: root.drawerWidth
            color: "#0f172a"
            border { color: "#1e293b"; width: 1 }

            // 顶部渐变线
            Rectangle {
                anchors { top: parent.top; left: parent.left; right: parent.right }
                height: 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#60a5fa" }
                    GradientStop { position: 1.0; color: "#22c55e" }
                }
            }

            // 标题栏
            Rectangle {
                id: titleBar
                anchors { top: parent.top; left: parent.left; right: parent.right }
                height: 56; color: "transparent"

                Text {
                    anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                    text: root.title
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 18; font.weight: Font.Bold; color: "#e2e8f0"
                }

                Rectangle {
                    anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                    width: 32; height: 32; radius: 6
                    color: closeMouse.containsMouse ? "#1e293b" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"; font.pixelSize: 16
                        color: closeMouse.containsMouse ? "#e2e8f0" : "#64748b"
                    }
                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }

                Rectangle {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                    height: 1; color: "#1e293b"
                }
            }

            // 内容区
            Item {
                id: contentArea
                anchors {
                    top: titleBar.bottom
                    left: parent.left; right: parent.right
                    bottom: btnBar.top
                    margins: 20
                }
            }

            // 底部按钮栏
            Rectangle {
                id: btnBar
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                height: 72; color: "transparent"

                Rectangle {
                    anchors { left: parent.left; right: parent.right; top: parent.top }
                    height: 1; color: "#1e293b"
                }

                RowLayout {
                    anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
                    spacing: 12

                    // 取消按钮
                    Rectangle {
                        implicitWidth: 100; implicitHeight: 38
                        radius: 6
                        color: cancelMouse.containsMouse ? "#7f1d1d" : "#450a0a"
                        border { color: cancelMouse.containsMouse ? "#f87171" : "#ef4444"; width: 1 }
                        Text {
                            anchors.centerIn: parent
                            text: "取消"; font.pixelSize: 14; font.weight: Font.Medium
                            color: cancelMouse.containsMouse ? "#fca5a5" : "#ef4444"
                        }
                        MouseArea {
                            id: cancelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { root.cancelled(); root.close() }
                        }
                    }

                    // 保存按钮
                    Rectangle {
                        implicitWidth: 100; implicitHeight: 38
                        radius: 6
                        color: saveMouse.containsMouse ? "#166534" : "#14532d"
                        border { color: saveMouse.containsMouse ? "#4ade80" : "#22c55e"; width: 1 }
                        Text {
                            anchors.centerIn: parent
                            text: "保存"; font.pixelSize: 14; font.weight: Font.Medium
                            color: saveMouse.containsMouse ? "#4ade80" : "#22c55e"
                        }
                        MouseArea {
                            id: saveMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { root.saved(); root.close() }
                        }
                    }
                }
            }
        }
    }

    // ===== 动画（使用 contentItem 的父级尺寸） =====
    enter: Transition {
        SequentialAnimation {
            NumberAnimation { target: bgMask; property: "opacity"; from: 0; to: 1; duration: 200 }
            NumberAnimation { target: panel; property: "x"; from: root.implicitWidth; to: root.implicitWidth - root.drawerWidth; duration: 280; easing.type: Easing.OutCubic }
        }
    }
    exit: Transition {
        SequentialAnimation {
            NumberAnimation { target: panel; property: "x"; from: root.implicitWidth - root.drawerWidth; to: root.implicitWidth; duration: 220; easing.type: Easing.InCubic }
            NumberAnimation { target: bgMask; property: "opacity"; from: 1; to: 0; duration: 150 }
        }
    }
}
