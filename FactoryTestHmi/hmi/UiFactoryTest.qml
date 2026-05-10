import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
Rectangle {
    color: "transparent"
    UiTestCard {
        id: testCard
        width: 480
        height: 550
        color: "#0f172a"
        radius: 10
        antialiasing: true
    }

    Rectangle {
        id: serialCard
        anchors.top: testCard.bottom
        anchors.topMargin: 16
        anchors.left: testCard.left
        width: 480
        height: 370
        radius: 12
        color: "transparent"

        // 背景
        Rectangle {
            anchors.fill: parent
            radius: 12
            border { color: "#1e293b"; width: 1 }
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0f172a" }
                GradientStop { position: 1.0; color: "#020617" }
            }
        }

        ColumnLayout {
            anchors {
                left: parent.left; right: parent.right
                top: parent.top; bottom: parent.bottom
            }
            spacing: 8

            // 工厂配置
            RowLayout {
                Layout.leftMargin: 25
                Layout.topMargin: 8
                Layout.fillWidth: true; spacing: 8
                Canvas {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.beginPath()
                        ctx.moveTo(3, 0.5); ctx.lineTo(15, 0.5)
                        ctx.arcTo(17.5, 0.5, 17.5, 3, 2.5)
                        ctx.lineTo(17.5, 15); ctx.arcTo(17.5, 17.5, 15, 17.5, 2.5)
                        ctx.lineTo(3, 17.5); ctx.arcTo(0.5, 17.5, 0.5, 15, 2.5)
                        ctx.lineTo(0.5, 3); ctx.arcTo(0.5, 0.5, 3, 0.5, 2.5)
                        ctx.closePath()
                        ctx.fillStyle = "#7c2d12"
                        ctx.fill()
                        ctx.strokeStyle = "#9a3412"
                        ctx.lineWidth = 1
                        ctx.stroke()

                        ctx.beginPath(); ctx.arc(6, 5.8, 1.8, 0, Math.PI*2); ctx.fillStyle = "#fed7aa"; ctx.fill()
                        ctx.beginPath(); ctx.arc(12, 5.8, 1.8, 0, Math.PI*2); ctx.fillStyle = "#fb923c"; ctx.fill()
                        ctx.beginPath(); ctx.arc(6, 11.8, 1.8, 0, Math.PI*2); ctx.fillStyle = "#f97316"; ctx.fill()
                        ctx.beginPath(); ctx.arc(12, 11.8, 1.8, 0, Math.PI*2); ctx.fillStyle = "#ea580c"; ctx.fill()
                    }
                    Component.onCompleted: requestPaint()
                }
                Text { text: "所属项目: "; font.pixelSize: 19; color: "#94a3b8" }
                Text { text: "P39"; font.pixelSize: 21; color: "#e2e8f0" }
            }

            Rectangle {
                Layout.fillWidth: true; height: 1; color: "#1e293b"
            }

            // ===== 串口配置区 (已调整好) =====
            // ===== 串口配置区 =====
            ColumnLayout {
                Layout.fillWidth: true; spacing: 25

                // --- COM 口 (重新绘制，更饱满) ---
                RowLayout {
                    Layout.leftMargin: 80
                    Layout.fillWidth: true; spacing: 8
                    Canvas {
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0,0,18,18)
                            ctx.strokeStyle = "#60a5fa"
                            ctx.lineWidth = 1.5
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            // 外框 (D-sub 形状简化)
                            ctx.beginPath()
                            ctx.moveTo(3, 2)
                            ctx.lineTo(15, 2)
                            ctx.arcTo(16, 2, 16, 3, 1)
                            ctx.lineTo(16, 11)
                            ctx.arcTo(16, 16, 13, 16, 3)
                            ctx.lineTo(5, 16)
                            ctx.arcTo(2, 16, 2, 13, 3)
                            ctx.lineTo(2, 3)
                            ctx.arcTo(2, 2, 3, 2, 1)
                            ctx.closePath()
                            ctx.stroke()
                            // 上排针脚 (2个)
                            ctx.fillStyle = "#60a5fa"
                            ctx.fillRect(5, 5.5, 3, 2.5)
                            ctx.fillRect(10, 5.5, 3, 2.5)
                            // 下排针脚 (2个)
                            ctx.fillRect(5, 10.5, 3, 2.5)
                            ctx.fillRect(10, 10.5, 3, 2.5)
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text { text: "   COM口:          "; font.pixelSize: 15; color: "#94a3b8" }
                    Text { text: "PVT01"; font.pixelSize: 18; color: "#e2e8f0" }
                }

                // --- 波特率 (原指针风格，稍微放大占满画布) ---
                RowLayout {
                    Layout.leftMargin: 80
                    Layout.fillWidth: true; spacing: 8
                    Canvas {
                        width: 20; height: 20
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0,0,width,height)
                            var cx=10, cy=10, r=7.5
                            ctx.strokeStyle="#f59e0b"; ctx.lineWidth=1.8; ctx.lineCap="round"
                            ctx.fillStyle="#f59e0b"
                            // 外弧（半圆）
                            ctx.beginPath()
                            ctx.arc(cx, cy, r, Math.PI, 2*Math.PI, false)
                            ctx.stroke()
                            // 左右刻度
                            ctx.beginPath()
                            ctx.moveTo(cx - r + 1.5, cy - 1.5)
                            ctx.lineTo(cx - r + 1.5, cy + 1.5)
                            ctx.moveTo(cx + r - 1.5, cy - 1.5)
                            ctx.lineTo(cx + r - 1.5, cy + 1.5)
                            ctx.stroke()
                            // 底部水平线
                            ctx.beginPath()
                            ctx.moveTo(cx - r, cy)
                            ctx.lineTo(cx + r, cy)
                            ctx.stroke()
                            // 中心指针
                            ctx.beginPath()
                            ctx.moveTo(cx, cy)
                            ctx.lineTo(cx-3.5, cy - r*0.7)
                            ctx.lineTo(cx+3.5, cy - r*0.7)
                            ctx.closePath()
                            ctx.fill()
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text { text: "    波特率:          "; font.pixelSize: 15; color: "#94a3b8" }
                    Text { text: "912600"; font.pixelSize:18; color: "#e2e8f0" }
                }

                // --- MES IP (更清晰的地球，加大网格) ---
                RowLayout {
                    Layout.leftMargin: 80
                    Layout.fillWidth: true; spacing: 8
                    Canvas {
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0,0,18,18)
                            ctx.strokeStyle = "#8b5cf6"
                            ctx.lineWidth = 1.6
                            ctx.lineCap = "round"
                            // 外圆
                            ctx.beginPath(); ctx.arc(9,9,7.2,0,2*Math.PI); ctx.stroke()
                            // 经线
                            ctx.beginPath(); ctx.moveTo(9,2); ctx.lineTo(9,16); ctx.stroke()
                            // 纬线（强化）
                            ctx.beginPath(); ctx.moveTo(2.5,9); ctx.lineTo(15.5,9); ctx.stroke()
                            // 两条斜线，更靠近边界
                            ctx.beginPath(); ctx.moveTo(5,3); ctx.lineTo(13,15); ctx.stroke()
                            ctx.beginPath(); ctx.moveTo(13,3); ctx.lineTo(5,15); ctx.stroke()
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text { text: "    mes_ip:         "; font.pixelSize: 15; color: "#94a3b8" }
                    Text { text: "192.168.60.168"; font.pixelSize: 18; color: "#e2e8f0" }
                }

                // --- MES 端口 (插头更宽，占用更多画布) ---
                // --- MES 端口 (加宽插头，两侧金属片贴边) ---
                RowLayout {
                    Layout.leftMargin: 80
                    Layout.fillWidth: true; spacing: 8
                    Canvas {
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0,0,18,18)
                            ctx.strokeStyle = "#a78bfa"
                            ctx.lineWidth = 1.5
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            // 两侧金属片直接贴到左右边缘
                            ctx.beginPath(); ctx.moveTo(3,5); ctx.lineTo(3,12); ctx.stroke()
                            ctx.beginPath(); ctx.moveTo(15,5); ctx.lineTo(15,12); ctx.stroke()
                            // 底部连接线从边缘延伸
                            ctx.beginPath(); ctx.moveTo(2,12); ctx.lineTo(7,14.5); ctx.lineTo(11,14.5); ctx.lineTo(16,12); ctx.stroke()
                            // 顶部横条加长
                            ctx.fillStyle = "#a78bfa"
                            ctx.fillRect(5,2.5,8,2.8)
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text { text: "  mes端口:         "; font.pixelSize: 15; color: "#94a3b8" }
                    Text { text: "8080"; font.pixelSize: 18; color: "#e2e8f0" }
                }

                // --- MES 上传 (云朵完全撑开，端点直达边缘) ---
                RowLayout {
                    Layout.leftMargin: 80
                    Layout.fillWidth: true; spacing: 8
                    Canvas {
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0,0,18,18)
                            var active = mesToggle.checked
                            ctx.strokeStyle = active ? "#22c55e" : "#475569"
                            ctx.fillStyle = active ? "#22c55e" : "#475569"
                            ctx.lineWidth = 1.5
                            ctx.lineCap = "round"; ctx.lineJoin = "round"
                            // 云轮廓：由左右两个拱形和一个宽阔的平底组成
                            // 左侧弧线
                            ctx.beginPath()
                            ctx.moveTo(2, 12)
                            ctx.arcTo(2, 6, 6, 6, 4)
                            // 中间弧线
                            ctx.arcTo(9, 2, 12, 6, 4)
                            // 右侧弧线
                            ctx.arcTo(16, 6, 16, 12, 4)
                            ctx.lineTo(16, 12)
                            ctx.stroke()
                            // 底部平盘 (充满底部区域)
                            ctx.beginPath()
                            ctx.moveTo(4, 12)
                            ctx.lineTo(4, 15)
                            ctx.lineTo(14, 15)
                            ctx.lineTo(14, 12)
                            ctx.stroke()
                            // 顶部圆点替换为小闪电/箭头 (表示上传)，更生动
                            ctx.beginPath()
                            ctx.moveTo(9, 3)
                            ctx.lineTo(9, 7.5)
                            ctx.moveTo(7, 5.5)
                            ctx.lineTo(9, 8.5)
                            ctx.lineTo(11, 5.5)
                            ctx.stroke()
                            ctx.fill()
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text { text: "  mes上传:         "; font.pixelSize: 15; color: "#94a3b8" }
                    Text {
                        text: mesToggle.checked ? "已开启" : "已关闭"
                        font.pixelSize: 18; color: mesToggle.checked ? "#22c55e" : "#64748b"
                    }

                    // 你的切换按钮，完全保留
                    Rectangle {
                        id: mesToggle
                        property bool checked: true
                        width: 44; height: 24; radius: 12
                        color: checked ? "#166534" : "#1e293b"
                        border { color: checked ? "#22c55e" : "#334155"; width: 1 }

                        Rectangle {
                            width: 18; height: 18; radius: 9
                            x: mesToggle.checked ? mesToggle.width - 20 : 2
                            y: 3; color: mesToggle.checked ? "#22c55e" : "#64748b"
                            Behavior on x { NumberAnimation { duration: 150 } }
                        }

                        MouseArea { anchors.fill: parent; onClicked: mesToggle.checked = !mesToggle.checked }
                    }
                }
            }
        }
    }

    //站位信息
    Rectangle {
        id:staionInfo
        anchors.top: testCard.top
        anchors.left: testCard.right
        anchors.leftMargin: 16
        width: 1392
        height: 50
        color: "#0f172a"
        radius: 10
        antialiasing: true
    }

    Rectangle {
        id: statisticsCard
        anchors.top: staionInfo.bottom
        anchors.topMargin: 16
        anchors.left: testCard.right
        anchors.leftMargin: 16
        width: 1392
        height: 70
        color: "#0f172a"
        radius: 10
        antialiasing: true

        // 分隔线小组件（方便重复使用）
        component StatItem : Column {
            spacing: 4
            property alias label: labelText.text
            property alias value: valueText.text
            property alias valueColor: valueText.color

            Text {
                id: labelText
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 15
                color: "#64748b"       // text-slate-500
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                id: valueText
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 36
                font.weight: Font.Bold
                font.letterSpacing: 1
                color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        //统计框
        RowLayout {
            Layout.preferredHeight: 60
            Layout.fillWidth: true   // 填满宽度
            spacing: 30
            // 四个统计项
            Item { Layout.fillWidth: true; implicitWidth: 0 }
            StatItem {
                label: "总量"
                value: "1,250"
                valueColor: "white"
            }
            Rectangle { width: 1; height: 40; color: "#1e293b"; anchors.verticalCenter: parent.verticalCenter }  // 分隔线

            StatItem {
                label: "成功"
                value: "1,231"
                valueColor: "#22c55e"      // green-500
            }
            Rectangle { width: 1; height: 40; color: "#1e293b"; anchors.verticalCenter: parent.verticalCenter }

            StatItem {
                label: "失败"
                value: "19"
                valueColor: "#ef4444"      // red-500
            }
            Rectangle { width: 1; height: 40; color: "#1e293b"; anchors.verticalCenter: parent.verticalCenter }

            StatItem {
                label: "合格率"
                value: "98.48%"
                valueColor: "#60a5fa"      // blue-400
            }
            Item { Layout.fillWidth: true; implicitWidth: 0 }
        }
    }

    UiFactoryTestRtTable {
        id:tableCard
        anchors.top: statisticsCard.bottom
        anchors.topMargin: 8
        anchors.left: testCard.right
        anchors.leftMargin: 16
        width: 1392
        height: 792
        //color: "#0f172a"
        //radius: 10
        antialiasing: true

        tableModel: rtModel
    }
}