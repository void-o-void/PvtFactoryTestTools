// UiFactoryTestRtTable.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0b1120"
    radius: 8
    border.color: "#1e293b"
    border.width: 1

    property var tableModel
    readonly property var colHeaders: ["名称", "测试码", "信息", "状态", "时长", "结果"]

    // 1:1:4:1:1:1，用整数避浮点累加误差
    function colW(total, idx) {
        var u = Math.floor(total / 9)
        return (idx === 2) ? total - u * 5 : u
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ========== 表头 ==========
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#0f172a"
            border.color: "#1e293b"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 0

                Repeater {
                    model: colHeaders
                    delegate: Text {
                        Layout.preferredWidth: colW(parent.width, index)
                        Layout.fillHeight: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: modelData
                        font.pixelSize: 13; font.weight: Font.Medium
                        color: "#64748b"
                    }
                }
            }
        }

        // ========== 分隔线 ==========
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e293b" }

        // ========== 表体 ==========
        TableView {
            id: tv
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            clip: true
            model: root.tableModel
            rowSpacing: 0; columnSpacing: 0
            boundsBehavior: Flickable.StopAtBounds

            columnWidthProvider: function(c) { return colW(tv.width, c) }
            rowHeightProvider: function(r) { return 48 }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded; width: 6
                contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#334155" }
                background: Rectangle { color: "transparent" }
            }

            delegate: Item {
                // 行背景
                Rectangle {
                    anchors.fill: parent
                    color: model.status === "processing" ? "#0a1628" : "transparent"
                }
                // 行底边线
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 1
                    color: "#1e293b"
                }

                // ===== 状态列：图标 + 文字 =====
                RowLayout {
                    anchors.centerIn: parent
                    visible: column === 3
                    spacing: 6

                    Rectangle {
                        visible: model.status === "finished"
                        width: 18; height: 18; radius: 9; color: "#22c55e"
                        Text { anchors.centerIn: parent; text: "✓"; color: "white"; font.pixelSize: 11; font.weight: Font.Bold }
                    }
                    Text {
                        visible: model.status === "processing"
                        width: 18; height: 18; text: "\u27F3"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 16; color: "#3b82f6"
                        RotationAnimator on rotation {
                            from: 0; to: 360; duration: 1000; loops: Animation.Infinite
                            running: model.status === "processing"
                        }
                    }
                    Rectangle {
                        visible: model.status === "waiting"
                        width: 18; height: 18; radius: 9; color: "transparent"
                        border { color: "#475569"; width: 1.5 }
                        Text { anchors.centerIn: parent; text: "⏳"; font.pixelSize: 10; color: "#64748b" }
                    }
                    Text {
                        text: model.status === "finished" ? "已完成" :
                              model.status === "processing" ? "进行中" : "等待中"
                        color: model.status === "finished" ? "#22c55e" :
                               model.status === "processing" ? "#3b82f6" : "#64748b"
                        font.pixelSize: 14
                    }
                }

                // ===== 普通文本列 =====
                Text {
                    anchors.fill: parent
                    visible: column !== 3
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    padding: 4

                    function columnText() {
                        switch (column) {
                            case 0: return model.name || ""
                            case 1: return model.testCode || ""
                            case 2: return model.message || ""
                            case 4: return model.duration || ""
                            case 5: return model.result === "pass" ? "PASS" :
                                           model.result === "fail" ? "FAIL" : "--"
                        }
                        return ""
                    }
                    text: columnText()

                    function columnColor() {
                        if (column === 5)
                            return model.result === "pass" ? "#22c55e" :
                                   model.result === "fail" ? "#ef4444" : "#64748b"
                        if (column === 2) return "#94a3b8"
                        return "#e2e8f0"
                    }
                    color: columnColor()

                    font.pixelSize: column === 2 ? 13 : 14
                    font.weight: column === 5 ? Font.Bold : Font.Normal
                    font.family: (column === 1 || column === 4) ? "Consolas, monospace" : ""
                }
            }
        }
    }
}
