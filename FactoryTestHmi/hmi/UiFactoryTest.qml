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

    // Rectangle {
    //     anchors.top: testCard.bottom
    //     anchors.topMargin: 16
    //     width: 480
    //     height: 160
    //     color: "#0f172a"
    //     radius: 10
    //     antialiasing: true
    // }
    // ---------- 串口通讯设置卡片 ----------
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

            //工厂配置
            RowLayout {
                Layout.leftMargin: 25   // 调整这里的数值来设定左边距
                Layout.topMargin: 8     // 调整这里的数值来设定上边距
                Layout.fillWidth: true; spacing: 8
                Canvas {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20

                    onPaint: {
                        var ctx = getContext("2d")

                        // ── 深色圆角背景 (暖色基底) ──────────
                        ctx.beginPath()
                        ctx.moveTo(3, 0.5)
                        ctx.lineTo(15, 0.5)
                        ctx.arcTo(17.5, 0.5, 17.5, 3, 2.5)
                        ctx.lineTo(17.5, 15)
                        ctx.arcTo(17.5, 17.5, 15, 17.5, 2.5)
                        ctx.lineTo(3, 17.5)
                        ctx.arcTo(0.5, 17.5, 0.5, 15, 2.5)
                        ctx.lineTo(0.5, 3)
                        ctx.arcTo(0.5, 0.5, 3, 0.5, 2.5)
                        ctx.closePath()

                        ctx.fillStyle = "#7c2d12"           // 深橙色背景 (orange-900)
                        ctx.fill()
                        ctx.strokeStyle = "#9a3412"         // 稍亮的橙色边框 (orange-800)
                        ctx.lineWidth = 1
                        ctx.stroke()

                        // ── 2×2 橙色圆点矩阵 (对角线渐变) ──
                        // 左上圆点 (最亮)
                        ctx.beginPath()
                        ctx.arc(6, 5.8, 1.8, 0, Math.PI * 2)
                        ctx.fillStyle = "#fed7aa"           // 浅橙 (orange-200)
                        ctx.fill()

                        // 右上圆点 (中亮)
                        ctx.beginPath()
                        ctx.arc(12, 5.8, 1.8, 0, Math.PI * 2)
                        ctx.fillStyle = "#fb923c"           // 中橙 (orange-400)
                        ctx.fill()

                        // 左下圆点 (中深)
                        ctx.beginPath()
                        ctx.arc(6, 11.8, 1.8, 0, Math.PI * 2)
                        ctx.fillStyle = "#f97316"           // 标准橙 (orange-500)
                        ctx.fill()

                        // 右下圆点 (最深)
                        ctx.beginPath()
                        ctx.arc(12, 11.8, 1.8, 0, Math.PI * 2)
                        ctx.fillStyle = "#ea580c"           // 深橙 (orange-600)
                        ctx.fill()
                    }

                    Component.onCompleted: requestPaint()
                }
                Text { text: "所属项目: "; font.pixelSize: 19; color: "#94a3b8" }
                Text { text: "P39"; font.pixelSize: 21; color: "#e2e8f0" }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1e293b"
            }

            // ===== 串口配置区 =====
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10

                // COM 口
                RowLayout {
                    Layout.leftMargin: 30   // 调整这里的数值来设定左边距
                    Layout.fillWidth: true;
                    spacing: 8
                    Canvas {
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.fillStyle = "#60a5fa"
                            ctx.fillRect(2,4,14,3); ctx.fillRect(2,9,14,3)
                            ctx.fillRect(5,1,3,14); ctx.fillRect(11,1,3,14)
                            ctx.fillStyle = "#0f172a"; ctx.fillRect(7,5,4,6)
                        }
                        Component.onCompleted: requestPaint()
                    }

                    Text { text: "COM口: "; font.pixelSize: 15; color: "#94a3b8" }
                    Text { text: "PVT01"; font.pixelSize: 18; color: "#e2e8f0" }
                }

                // 波特率
                RowLayout {
                    Layout.leftMargin: 30   // 调整这里的数值来设定左边距
                    Layout.fillWidth: true; spacing: 8
                    Canvas {
                        width: 20
                        height: 20
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)

                            var cx = width / 2, cy = height / 2
                            var r = width * 0.38       // 外弧半径
                            var thickness = width * 0.15

                            ctx.strokeStyle = "#f59e0b"
                            ctx.lineWidth = 1.5
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            ctx.fillStyle = "#f59e0b"

                            // 1. 外弧（半圆环形）
                            ctx.beginPath()
                            ctx.arc(cx, cy, r, Math.PI, 2 * Math.PI, false)
                            ctx.stroke()

                            // 2. 内刻度短线（左侧和右侧各一条）
                            ctx.beginPath()
                            // 左侧刻度
                            ctx.moveTo(cx - r + 3, cy - 1)
                            ctx.lineTo(cx - r + 3, cy + 1)
                            // 右侧刻度
                            ctx.moveTo(cx + r - 3, cy - 1)
                            ctx.lineTo(cx + r - 3, cy + 1)
                            ctx.stroke()

                            // 3. 底部水平直线
                            ctx.beginPath()
                            ctx.moveTo(cx - r + 1, cy)
                            ctx.lineTo(cx + r - 1, cy)
                            ctx.stroke()

                            // 4. 中心指针（三角形）
                            ctx.beginPath()
                            ctx.moveTo(cx, cy)               // 圆心
                            ctx.lineTo(cx - 3, cy - r * 0.7) // 左上
                            ctx.lineTo(cx + 3, cy - r * 0.7) // 右上
                            ctx.closePath()
                            ctx.fill()
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text { text: "波特率: "; font.pixelSize: 15; color: "#94a3b8" }
                    Text { text: " 912600"; font.pixelSize:18; color: "#e2e8f0" }
                }
            }

            // 分隔线
            Rectangle { Layout.fillWidth: true; height: 1; color: "#1e293b" }

            // ===== MES 配置区 =====
            ColumnLayout {
                Layout.fillWidth: true; spacing: 10

                // MES IP
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: "MES IP"; font.pixelSize: 13; color: "#94a3b8" }
                    TextField {
                        id: mesIpInput; Layout.fillWidth: true; font.pixelSize: 14; color: "#e2e8f0"
                        placeholderText: "192.168.x.x"; placeholderTextColor: "#475569"
                        background: Item {}
                        Rectangle {
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.bottom: parent.bottom; height: 1
                            color: parent.activeFocus ? "#60a5fa" : "#475569"
                        }
                    }
                }

                // MES 端口
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: "MES端口"; font.pixelSize: 13; color: "#94a3b8" }
                    TextField {
                        id: mesPortInput; Layout.fillWidth: true; font.pixelSize: 14; color: "#e2e8f0"
                        text: "8080"; background: Item{}
                        Rectangle {
                            anchors.left: parent.left; anchors.right: parent.right
                            anchors.bottom: parent.bottom; height: 1
                            color: parent.activeFocus ? "#60a5fa" : "#475569"
                        }
                    }
                }

                // MES 上传开关
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    // 云上传图标
                    Canvas {
                        Layout.preferredWidth: 18; Layout.preferredHeight: 18
                        onPaint: {
                            var ctx = getContext("2d"); ctx.strokeStyle = mesToggle.checked ? "#22c55e" : "#475569"; ctx.lineWidth = 1.5
                            ctx.beginPath(); ctx.moveTo(4,10); ctx.lineTo(9,5); ctx.lineTo(14,10); ctx.stroke()
                            ctx.beginPath(); ctx.moveTo(6,10); ctx.lineTo(6,15); ctx.lineTo(12,15); ctx.lineTo(12,10); ctx.stroke()
                            ctx.fillStyle = mesToggle.checked ? "#22c55e" : "#475569"
                            ctx.beginPath(); ctx.arc(9,3,2,0,6.28); ctx.fill()
                        }
                        Component.onCompleted: requestPaint()
                    }
                    Text { text: "MES上传"; font.pixelSize: 13; color: "#94a3b8" }
                    Item { Layout.fillWidth: true } // 弹性空间

                    // 开关按钮
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

                        MouseArea {
                            anchors.fill: parent
                            onClicked: mesToggle.checked = !mesToggle.checked
                        }
                    }

                    Text {
                        text: mesToggle.checked ? "已开启" : "已关闭"
                        font.pixelSize: 12; color: mesToggle.checked ? "#22c55e" : "#64748b"
                    }
                }
            }
        }
    }


    Rectangle {
        anchors.top: testCard.top
        anchors.left: testCard.right
        anchors.leftMargin: 16
        width: 1392
        height: 936
        color: "#0f172a"
        radius: 10
        antialiasing: true
    }
}