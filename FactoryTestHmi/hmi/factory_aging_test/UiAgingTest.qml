import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "transparent"

    property int maxTempPoints: 120
    property var tempApNtc:  []
    property var tempMd:     []
    property var tempPmic:   []
    property var tempSoc:    []
    property int chartTick: 0
    property real cpuLoad: 0; property real gpuLoad: 0; property real apuLoad: 0
    property real freqVal: 0

    function pushTemp(apNtc, md, pmic, soc) {
        function push(arr, v) { arr.push(v); if (arr.length > maxTempPoints) arr.shift() }
        push(tempApNtc, apNtc)
        push(tempMd, md)
        push(tempPmic, pmic)
        push(tempSoc, soc)
        chartTick++
    }

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

    Connections {
        target: agingTestManage
        function onLogMessage(msg) { logCard.append(msg) }
        function onHandshakeDone() { agingCard.agingPhase = "configuring" }
        function onConfigDone()    { agingCard.agingPhase = "testing" }
        function onTempDataUpdated(apNtc, md, pmic, soc) { pushTemp(apNtc, md, pmic, soc) }
        function onDashboardDataUpdated(cpu, gpu, apu, freq) {
            cpuLoad = cpu; gpuLoad = gpu; apuLoad = apu; freqVal = freq
        }
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

    // ===== 仪表盘：4 等分（负载×3 + 频率×1） =====
    Rectangle {
        id: dashBoard
        anchors.top: staionInfo.bottom; anchors.topMargin: 16
        anchors.left: agingCard.right; anchors.leftMargin: 16
        width: 1392; height: 240; color: "#0f172a"; radius: 10; antialiasing: true

        RowLayout {
            anchors.fill: parent; anchors.margins: 12
            spacing: 0

            // CPU 负载
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                WaterBall { anchors.centerIn: parent; percent: cpuLoad; label: "CPU 负载"; waterColor: "#60a5fa" }
            }
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; Layout.topMargin: 20; Layout.bottomMargin: 20; color: "#1e293b" }

            // GPU 负载
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                WaterBall { anchors.centerIn: parent; percent: gpuLoad; label: "GPU 负载"; waterColor: "#22c55e" }
            }
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; Layout.topMargin: 20; Layout.bottomMargin: 20; color: "#1e293b" }

            // APU 负载
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                WaterBall { anchors.centerIn: parent; percent: apuLoad; label: "APU 负载"; waterColor: "#f59e0b" }
            }
            Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; Layout.topMargin: 20; Layout.bottomMargin: 20; color: "#1e293b" }

            // DRAM 频率
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                FreqGauge { anchors.centerIn: parent; freqValue: freqVal; accentColor: "#a78bfa" }
            }
        }
    }

    // ===== 温度曲线图 =====
    UiLineChart {
        id: tempChart
        anchors.top: dashBoard.bottom; anchors.topMargin: 16
        anchors.left: agingCard.right; anchors.leftMargin: 16
        anchors.bottom: parent.bottom
        width: 1392

        windowSec: maxTempPoints
        series: { chartTick; return ({ "apNtcTemp": tempApNtc, "mdTemp": tempMd, "pmicTemp": tempPmic, "socMaxTemp": tempSoc }) }
        colors: ({ "apNtcTemp": "#60a5fa", "mdTemp": "#f59e0b", "pmicTemp": "#22c55e", "socMaxTemp": "#ef4444" })
        labels: ({ "apNtcTemp": "板端",  "mdTemp": "MD",    "pmicTemp": "PMIC",  "socMaxTemp": "SOC" })
    }
}
