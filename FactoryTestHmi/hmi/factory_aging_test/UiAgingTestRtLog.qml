import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    color: "transparent"
    radius: 12

    // C++ 通过 append(line) 推入日志行
    property var logModel: ListModel { id: logList }

    function append(line) {
        logList.append({ text: line })
        if (logList.count > 500) logList.remove(0)  // 保留最近 500 条
    }

    function clear() { logList.clear() }

    // 背景
    Rectangle {
        anchors.fill: parent; radius: 12
        border { color: "#1e293b"; width: 1 }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0f172a" }
            GradientStop { position: 1.0; color: "#020617" }
        }
    }

    ColumnLayout {
        anchors { fill: parent; margins: 16 }
        spacing: 8

        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            Text { text: "实时日志"; font.pixelSize: 16; font.weight: Font.Bold; color: "#94a3b8" }
            Item { Layout.fillWidth: true }
            Text {
                text: logList.count + " 条"
                font.pixelSize: 12; color: "#64748b"
            }
            Rectangle {
                width: 60; height: 28; radius: 4; color: clearMouse.containsMouse ? "#1e293b" : "transparent"
                border { color: "#334155"; width: 1 }
                Text { anchors.centerIn: parent; text: "清空"; font.pixelSize: 12; color: clearMouse.containsMouse ? "#e2e8f0" : "#94a3b8" }
                MouseArea { id: clearMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.clear() }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#1e293b" }

        // 日志列表
        ListView {
            id: logView
            Layout.fillWidth: true; Layout.fillHeight: true
            model: logList
            clip: true
            spacing: 2
            boundsBehavior: Flickable.StopAtBounds

            // 新日志自动滚到底部（延迟一帧确保布局完成）
            onCountChanged: Qt.callLater(function() { positionViewAtEnd() })
            Component.onCompleted: positionViewAtEnd()

            ScrollBar.vertical: ScrollBar { width: 4; policy: ScrollBar.AsNeeded }

            delegate: Text {
                width: logView.width - 8
                text: model.text
                font.family: "Consolas, monospace"; font.pixelSize: 12
                color: {
                    if (model.text.indexOf("[ERR]") >= 0) return "#f87171"
                    if (model.text.indexOf("[WARN]") >= 0) return "#fbbf24"
                    if (model.text.indexOf("[OK]") >= 0) return "#4ade80"
                    return "#94a3b8"
                }
                lineHeight: 1.5
                wrapMode: Text.WrapAnywhere
            }
        }
    }
}
