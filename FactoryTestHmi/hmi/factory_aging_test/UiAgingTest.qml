import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "transparent"

    UiAgingTestCard {
        id: agingCard
        width: 480; height: 550
    }

    UiAgingTestRtLog {
        id: logCard
        anchors.top: agingCard.bottom
        anchors.topMargin: 16
        anchors.left: agingCard.left
        anchors.bottom: parent.bottom
        width: 480
    }

    // 老化管理器日志 → 实时日志区
    Connections {
        target: agingTestManage
        function onLogMessage(msg) { logCard.append(msg) }
    }

    // ===== 站位信息 =====
    Rectangle {
        id: staionInfo
        anchors.top: agingCard.top
        anchors.left: agingCard.right; anchors.leftMargin: 16
        width: 1392; height: 50; color: "#0f172a"; radius: 10; antialiasing: true

        readonly property real itemWidth: (width - 40 - 4) / 5

        component StationItem : Item {
            property alias label: labelText.text
            property alias value: valueText.text
            Row {
                spacing: 8; height: parent.height; anchors.centerIn: parent
                Text { id: labelText; height: parent.height; verticalAlignment: Text.AlignVCenter; font.family: "Inter, Segoe UI, sans-serif"; font.pixelSize: 14; color: "#64748b"; renderType: Text.NativeRendering }
                Text { id: valueText; height: parent.height; verticalAlignment: Text.AlignVCenter; font.family: "Inter, Segoe UI, sans-serif"; font.pixelSize: 17; font.weight: Font.Medium; color: "#e2e8f0"; renderType: Text.NativeRendering }
            }
        }

        StationItem { id: s1; anchors.left: parent.left; anchors.leftMargin: 20; anchors.verticalCenter: parent.verticalCenter; width: staionInfo.itemWidth; height: parent.height; label: "工单号"; value: "WO-20260510-0038" }
        Rectangle { anchors.left: s1.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
        StationItem { id: s2; anchors.left: s1.right; anchors.leftMargin: 1; anchors.verticalCenter: parent.verticalCenter; width: staionInfo.itemWidth; height: parent.height; label: "工序名称"; value: "老化测试" }
        Rectangle { anchors.left: s2.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
        StationItem { id: s3; anchors.left: s2.right; anchors.leftMargin: 1; anchors.verticalCenter: parent.verticalCenter; width: staionInfo.itemWidth; height: parent.height; label: "员工工号"; value: "EMP8823" }
        Rectangle { anchors.left: s3.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
        StationItem { id: s4; anchors.left: s3.right; anchors.leftMargin: 1; anchors.verticalCenter: parent.verticalCenter; width: staionInfo.itemWidth; height: parent.height; label: "线别"; value: "L3-2" }
        Rectangle { anchors.left: s4.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
        StationItem { id: s5; anchors.left: s4.right; anchors.leftMargin: 1; anchors.verticalCenter: parent.verticalCenter; width: staionInfo.itemWidth; height: parent.height; label: "夹具编号"; value: "FIX-017" }
    }

    // ===== 温度曲线图表区（预留） =====
    Rectangle {
        id: chartCard
        anchors.top: staionInfo.bottom; anchors.topMargin: 16
        anchors.left: agingCard.right; anchors.leftMargin: 16
        width: 1392; height: 250; color: "#0f172a"; radius: 10; antialiasing: true

        Text {
            anchors.centerIn: parent
            text: "温度曲线图表（待实现）"
            font.pixelSize: 20; color: "#475569"
        }
    }

    // ===== 表格区 =====
    Rectangle {
        id: tableCard
        anchors.top: chartCard.bottom; anchors.topMargin: 8
        anchors.left: agingCard.right; anchors.leftMargin: 16
        anchors.bottom: parent.bottom
        width: 1392
        color: "#0f172a"; radius: 10; antialiasing: true
        Text { anchors.centerIn: parent; text: "老化测试数据表格（待实现）"; font.pixelSize: 20; color: "#475569" }
    }
}
