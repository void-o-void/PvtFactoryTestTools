import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "transparent"

    // ===== 左侧面板 =====
    Rectangle {
        id: agingCard
        width: 480
        height: 550
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
                margins: 24
            }
            spacing: 16

            // 标题
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    width: 6; height: 28; radius: 3; color: "#f59e0b"
                }
                Text {
                    text: "老化测试控制台"
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 22; font.weight: Font.Bold
                    color: "#e2e8f0"
                }
            }

            Rectangle {
                Layout.fillWidth: true; height: 1; color: "#1e293b"
            }

            // 老化参数区
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 20

                // 老化时长
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text {
                        text: "\u23F0"; font.pixelSize: 20
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "老化时长:"
                        font.pixelSize: 15; color: "#94a3b8"
                    }
                    Text {
                        text: "2 小时"
                        font.pixelSize: 18; font.weight: Font.Medium
                        color: "#e2e8f0"
                    }
                    Item { Layout.fillWidth: true }
                }

                // 温度
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text {
                        text: "\u2103"; font.pixelSize: 20
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "目标温度:"
                        font.pixelSize: 15; color: "#94a3b8"
                    }
                    Text {
                        text: "85"
                        font.pixelSize: 18; font.weight: Font.Medium
                        color: "#e2e8f0"
                    }
                    Text {
                        text: "°C"
                        font.pixelSize: 14; color: "#64748b"
                    }
                    Item { Layout.fillWidth: true }
                }

                // 当前状态
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Rectangle {
                        width: 10; height: 10; radius: 5; color: "#f59e0b"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: "状态:"
                        font.pixelSize: 15; color: "#94a3b8"
                    }
                    Text {
                        text: "待启动"
                        font.pixelSize: 18; font.weight: Font.Medium
                        color: "#f59e0b"
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            Item { Layout.fillHeight: true }

            // 启动/停止按钮
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 8
                color: agingBtnMouse.containsMouse ? "#1e3a5f" : "#1e293b"
                border {
                    color: agingBtnMouse.containsMouse ? "#60a5fa" : "#334155"
                    width: 1
                }

                Text {
                    anchors.centerIn: parent
                    text: "\u25B6  开始老化测试"
                    font.pixelSize: 18; font.weight: Font.Medium
                    color: agingBtnMouse.containsMouse ? "#60a5fa" : "#cbd5e1"
                }

                MouseArea {
                    id: agingBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("老化测试启动")
                }
            }
        }
    }

    Rectangle {
        id: agingSerialCard
        anchors.top: agingCard.bottom
        anchors.topMargin: 16
        anchors.left: agingCard.left
        width: 480
        height: 370
        radius: 12
        color: "transparent"

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
                margins: 24
            }
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: "老化日志"
                font.pixelSize: 16; font.weight: Font.Bold
                color: "#94a3b8"
            }

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                radius: 6; color: "#020617"
                border { color: "#1e293b"; width: 1 }

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 12
                    contentWidth: parent.width - 24
                    contentHeight: logText.implicitHeight
                    clip: true

                    Text {
                        id: logText
                        width: parent.width
                        text: "[14:30:00] 老化测试系统就绪\n[14:30:01] 等待启动指令..."
                        font.family: "Consolas, monospace"
                        font.pixelSize: 12; color: "#64748b"
                        lineHeight: 1.6
                    }
                }
            }
        }
    }

    // ===== 站位信息 =====
    Rectangle {
        id: staionInfo
        anchors.top: agingCard.top
        anchors.left: agingCard.right
        anchors.leftMargin: 16
        width: 1392; height: 50
        color: "#0f172a"; radius: 10
        antialiasing: true

        readonly property real itemWidth: (width - 40 - 4) / 5

        component StationItem : Item {
            property alias label: labelText.text
            property alias value: valueText.text
            Row {
                spacing: 8
                height: parent.height
                anchors.centerIn: parent
                Text {
                    id: labelText
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 14; color: "#64748b"
                    renderType: Text.NativeRendering
                }
                Text {
                    id: valueText
                    height: parent.height
                    verticalAlignment: Text.AlignVCenter
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 17; font.weight: Font.Medium
                    color: "#e2e8f0"
                    renderType: Text.NativeRendering
                }
            }
        }

        StationItem {
            id: item1
            anchors.left: parent.left; anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            width: staionInfo.itemWidth; height: parent.height
            label: "工单号"; value: "WO-20260510-0038"
        }
        Rectangle {
            anchors.left: item1.right; anchors.verticalCenter: parent.verticalCenter
            width: 1; height: 28; color: "#1e293b"
        }
        StationItem {
            id: item2
            anchors.left: item1.right; anchors.leftMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            width: staionInfo.itemWidth; height: parent.height
            label: "工序名称"; value: "老化测试"
        }
        Rectangle {
            anchors.left: item2.right; anchors.verticalCenter: parent.verticalCenter
            width: 1; height: 28; color: "#1e293b"
        }
        StationItem {
            id: item3
            anchors.left: item2.right; anchors.leftMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            width: staionInfo.itemWidth; height: parent.height
            label: "员工工号"; value: "EMP8823"
        }
        Rectangle {
            anchors.left: item3.right; anchors.verticalCenter: parent.verticalCenter
            width: 1; height: 28; color: "#1e293b"
        }
        StationItem {
            id: item4
            anchors.left: item3.right; anchors.leftMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            width: staionInfo.itemWidth; height: parent.height
            label: "线别"; value: "L3-2"
        }
        Rectangle {
            anchors.left: item4.right; anchors.verticalCenter: parent.verticalCenter
            width: 1; height: 28; color: "#1e293b"
        }
        StationItem {
            id: item5
            anchors.left: item4.right; anchors.leftMargin: 1
            anchors.verticalCenter: parent.verticalCenter
            width: staionInfo.itemWidth; height: parent.height
            label: "夹具编号"; value: "FIX-017"
        }
    }

    // ===== 统计栏 =====
    Rectangle {
        id: statisticsCard
        anchors.top: staionInfo.bottom
        anchors.topMargin: 16
        anchors.left: agingCard.right
        anchors.leftMargin: 16
        width: 1392; height: 70
        color: "#0f172a"; radius: 10
        antialiasing: true

        component StatItem : Column {
            spacing: 4
            property alias label: labelText.text
            property alias value: valueText.text
            property alias valueColor: valueText.color
            Text {
                id: labelText
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 15; color: "#64748b"
                renderType: Text.NativeRendering
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                id: valueText
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 32; font.weight: Font.Bold
                font.letterSpacing: 1; color: "white"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        RowLayout {
            anchors.fill: parent; anchors.rightMargin: 16
            spacing: 0

            RowLayout {
                spacing: 100
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 100
                StatItem { label: "总量"; value: "0"; valueColor: "white" }
                Rectangle { width: 1; height: 40; color: "#1e293b" }
                StatItem { label: "运行中"; value: "0"; valueColor: "#f59e0b" }
                Rectangle { width: 1; height: 40; color: "#1e293b" }
                StatItem { label: "完成"; value: "0"; valueColor: "#22c55e" }
                Rectangle { width: 1; height: 40; color: "#1e293b" }
                StatItem { label: "异常"; value: "0"; valueColor: "#ef4444" }
            }
            Item { Layout.fillWidth: true }
        }
    }

    // ===== 老化测试表格区（占位） =====
    Rectangle {
        id: tableCard
        anchors.top: statisticsCard.bottom
        anchors.topMargin: 8
        anchors.left: agingCard.right
        anchors.leftMargin: 16
        width: 1392; height: 792
        color: "#0f172a"; radius: 10
        antialiasing: true

        Text {
            anchors.centerIn: parent
            text: "老化测试数据表格（待实现）"
            font.pixelSize: 20; color: "#475569"
        }
    }
}
