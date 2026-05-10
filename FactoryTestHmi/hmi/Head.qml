import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: topHeader
    width: 1920
    height: 96                  // 对应原始设计 h-24 (96px)
    radius: 12                  // rounded-xl
    antialiasing: true

    // 工业风格渐变背景
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#0f172a" }
        GradientStop { position: 1.0; color: "#020617" }
    }
    // 边框
    border.color: "#1e293b"
    border.width: 1
    // 内阴影用一层浅色半透明内边框模拟（可选）
    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "transparent"
        border.color: "#10ffffff"   // 很淡的白色内描边
        border.width: 1
    }

    // ----- 左侧：图标 + 标题区 -----
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 24       // px-6
        anchors.verticalCenter: parent.verticalCenter
        spacing: 16

        // 图标容器（蓝色方底）
        Rectangle {
            width: 48; height: 48
            radius: 8
            color: "#2563eb"          // bg-blue-600

            // 工厂图标（MDI factory）
            Shape {
                anchors.centerIn: parent
                width: 28; height: 24
                ShapePath {
                    fillColor: "white"
                    strokeColor: "transparent"
                    PathSvg {
                        path: "M19,11V3H23V11L22,21H2L1,11H5V3H9V11H10V3H14V11H15V3H19V11M17,13H7V19H17V13Z"
                    }
                }
            }
        }

        // 标题文字列
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            // 主标题 + 版本号行
            Row {
                spacing: 8
                Text {
                    text: "SMARTTEST PRO"
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 24
                    font.weight: Font.Bold
                    font.letterSpacing: 1.2   // tracking-wider (0.05em ≈ 1.2px)
                    color: "#60a5fa"           // text-blue-400
                }
                Text {
                    text: "V2.4.0"
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 14
                    font.weight: Font.Light
                    color: "#64748b"           // text-slate-500
                    anchors.baseline: parent.baseline
                }
            }

            // 副标题
            Text {
                text: "智能工厂测试集成系统"
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 12
                font.letterSpacing: 1.2        // tracking-widest (0.1em)
                color: "#94a3b8"              // text-slate-400
            }
        }
    }

    // ----- 右侧：统计数字 + 时间状态 -----
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 32
        anchors.verticalCenter: parent.verticalCenter
        spacing: 32
        // // 时间与状态区域
        // Rectangle { width: 1; height: 50; color: "#1e293b"; anchors.verticalCenter: parent.verticalCenter }

        Column {
            spacing: 6
            anchors.verticalCenter: parent.verticalCenter

            // 当前时间（每秒刷新）
            Text {
                id: clockText
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 20
                font.weight: Font.Bold
                color: "#cbd5e1"       // text-slate-300
                text: "2026-04-22 10:45:30"

                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: {
                        var now = new Date();
                        var Y = now.getFullYear();
                        var M = ("0" + (now.getMonth()+1)).slice(-2);
                        var D = ("0" + now.getDate()).slice(-2);
                        var h = ("0" + now.getHours()).slice(-2);
                        var m = ("0" + now.getMinutes()).slice(-2);
                        var s = ("0" + now.getSeconds()).slice(-2);
                        clockText.text = Y + "-" + M + "-" + D + " " + h + ":" + m + ":" + s;
                    }
                }
            }

            // 系统运行状态
            Row {
                spacing: 8
                anchors.horizontalCenter: parent.horizontalCenter

                // 绿色脉冲指示灯
                Rectangle {
                    width: 8; height: 8
                    radius: 4
                    color: "#22c55e"
                    anchors.verticalCenter: parent.verticalCenter

                    SequentialAnimation on opacity {
                        running: true
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                        NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                    }
                }

                Text {
                    text: "系统运行正常"
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 12
                    color: "#22c55e"       // green-500
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}