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
    readonly property var colHeaders: ["测试项", "测试码", "调试信息", "状态", "时长", "结果"]

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
            rowSpacing: 0
            columnSpacing: 0
            boundsBehavior: Flickable.StopAtBounds

            columnWidthProvider: function(c) { return colW(tv.width, c) }
            rowHeightProvider: function(r) { return 48 }

            ScrollBar.vertical: ScrollBar {
                id: vScrollBar
                policy: ScrollBar.AsNeeded
                width: 6

                // ✅ 关键：强制窗口背景（轨道）为透明
                palette.window: "transparent"

                opacity: nearHover.hovered || vScrollBar.pressed ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 200 } }

                // 背景保持透明 Rectangle（只是多一层保险）
                background: Rectangle {
                    implicitWidth: 6
                    color: "transparent"
                    HoverHandler {
                        id: nearHover
                    }
                }
            }

            delegate: Item {
                // 行背景（所有状态统一，无切换突兀感）
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                }
                // 行底边线
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 1
                    color: "#1e293b"
                }

                // ===== 状态列：4 状态图标 + 文字 =====
                RowLayout {
                    anchors.centerIn: parent
                    visible: column === 3
                    spacing: 6

                    // --- idle：灰色空心圆 + 横线 ---
                    Rectangle {
                        visible: model.status === "idle"
                        width: 18; height: 18; radius: 9
                        color: "transparent"
                        border { color: "#475569"; width: 1.5 }
                        Rectangle {
                            anchors.centerIn: parent
                            width: 6; height: 1.5; radius: 1; color: "#475569"
                        }
                    }

                    // --- standby：沙漏 ---
                    Text {
                        visible: model.status === "standby"
                        width: 18; height: 18
                        text: "\u23F3"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14; color: "#f59e0b"
                    }

                    // --- processing：绿色旋转圆环 ---
                    Canvas {
                        visible: model.status === "processing"
                        width: 18; height: 18
                        RotationAnimator on rotation {
                            from: 0; to: 360; duration: 800; loops: Animation.Infinite
                            running: model.status === "processing"
                        }
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = "#22c55e"
                            ctx.lineWidth = 2
                            ctx.lineCap = "round"
                            ctx.beginPath()
                            ctx.arc(9, 9, 6, Math.PI * 1.2, Math.PI * 0.8)
                            ctx.stroke()
                        }
                        Component.onCompleted: requestPaint()
                    }

                    // --- finished PASS：绿色圆 + ✓ ---
                    Rectangle {
                        visible: model.status === "finished" && model.result === "pass"
                        width: 18; height: 18; radius: 9; color: "#166534"
                        border { color: "#22c55e"; width: 1 }
                        Text {
                            anchors.centerIn: parent
                            text: "\u2713"; color: "#22c55e"
                            font.pixelSize: 11; font.weight: Font.Bold
                        }
                    }

                    // --- finished FAIL：红色圆 + ✗ ---
                    Rectangle {
                        visible: model.status === "finished" && model.result === "fail"
                        width: 18; height: 18; radius: 9; color: "#7f1d1d"
                        border { color: "#ef4444"; width: 1 }
                        Text {
                            anchors.centerIn: parent
                            text: "\u2717"; color: "#ef4444"
                            font.pixelSize: 11; font.weight: Font.Bold
                        }
                    }

                    // 文字
                    Text {
                        text: {
                            switch (model.status) {
                                case "idle":       return "未开始"
                                case "standby":    return "待测试"
                                case "processing": return "测试中"
                                case "finished":
                                    return model.result === "pass" ? "测试通过" : "测试不通过"
                            }
                            return ""
                        }
                        color: {
                            switch (model.status) {
                                case "idle":       return "#64748b"
                                case "standby":    return "#f59e0b"
                                case "processing": return "#22c55e"
                                case "finished":
                                    return model.result === "pass" ? "#22c55e" : "#ef4444"
                            }
                            return "#64748b"
                        }
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

                    // ✅ 关键修改 1：使用原生渲染，小字更清晰
                    renderType: Text.NativeRendering

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

                    // ✅ 关键修改 2：为不同列指定合适的字体
                    font.family: {
                        if (column === 1 || column === 4)          // 测试码、时长（等宽）
                            return "Consolas, monospace"
                        if (column === 0 || column === 5)          // 名称、结果（清晰可读）
                            return "system-ui, -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif"
                        return ""                                  // 信息列保持默认
                    }

                    font.pixelSize: column === 2 ? 13 : 14
                    // ✅ 关键修改 3：结果列粗体改为 DemiBold，避免笔画太肿
                    font.weight: column === 5 ? Font.DemiBold : Font.Normal
                }
            }
        }
    }
}
