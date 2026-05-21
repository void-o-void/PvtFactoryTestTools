import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string agingPhase: "idle"
    property string agingResult: ""
    property int elapsedSec: 0

    width: 480; height: 550
    color: "transparent"

    function formatTimer(s) {
        var h = Math.floor(s / 3600); var m = Math.floor((s % 3600) / 60); var sec = s % 60
        return String(h).padStart(2,'0') + ":" + String(m).padStart(2,'0') + ":" + String(sec).padStart(2,'0')
    }

    Timer {
        interval: 1000; running: root.agingPhase === "testing"; repeat: true
        onTriggered: root.elapsedSec++
    }

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
        anchors { left: parent.left; right: parent.right; top: parent.top; bottom: parent.bottom; topMargin: 12; bottomMargin: 24; leftMargin: 24; rightMargin: 24 }
        spacing: 10

        // 标题
        Text {
            Layout.fillWidth: true
            text: "老化测试控制台"
            font.family: "Inter, Segoe UI, sans-serif"; font.pixelSize: 20; font.weight: Font.Bold
            color: "#e2e8f0"
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#1e293b" }

        // ===== 圆形按钮 =====
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 256; Layout.preferredHeight: 256
            focusPolicy: Qt.NoFocus

            Canvas {
                anchors.fill: parent
                property string phase: root.agingPhase
                onPhaseChanged: requestPaint()
                Component.onCompleted: requestPaint()

                onPaint: {
                    var ctx = getContext("2d"); var cx = width/2, cy = height/2, r = width/2
                    ctx.clearRect(0,0,width,height)
                    ctx.beginPath(); ctx.arc(cx,cy,r,0,2*Math.PI); ctx.fillStyle = "#1e293b"; ctx.fill()
                    var grad = ctx.createRadialGradient(cx,cy,r*0.05,cx,cy,r-8)
                    if (agingPhase === "testing")      { grad.addColorStop(0,"#22c55e"); grad.addColorStop(1,"#14532d") }
                    else if (agingPhase === "waiting") { grad.addColorStop(0,"#f59e0b"); grad.addColorStop(1,"#78350f") }
                    else if (agingPhase === "finished") {
                        if (agingResult === "PASS") { grad.addColorStop(0,"#22c55e"); grad.addColorStop(1,"#064e3b") }
                        else { grad.addColorStop(0,"#ef4444"); grad.addColorStop(1,"#7f1d1d") }
                    } else { grad.addColorStop(0,"#1e293b"); grad.addColorStop(1,"#0f172a") }
                    ctx.beginPath(); ctx.arc(cx,cy,r-8,0,2*Math.PI); ctx.fillStyle = grad; ctx.fill()
                }
            }

            // 中心图标 + 文字
            Column {
                anchors.centerIn: parent; spacing: 8
                Item {
                    anchors.horizontalCenter: parent.horizontalCenter; width: 55; height: 65

                    Canvas {
                        anchors.centerIn: parent; visible: agingPhase === "idle"; width: 55; height: 65
                        onPaint: { var c=getContext("2d"); c.fillStyle="#94a3b8"; c.beginPath(); c.moveTo(8,3); c.lineTo(width,height/2); c.lineTo(8,height-3); c.closePath(); c.fill() }
                        Component.onCompleted: requestPaint()
                    }

                    Row {
                        anchors.centerIn: parent; visible: agingPhase === "waiting"; spacing: 12
                        Repeater {
                            model: 3
                            Rectangle {
                                width: 12; height: 12; radius: 6
                                color: ["#fbbf24","#f59e0b","#d97706"][index]
                                SequentialAnimation on opacity { loops: Animation.Infinite; PauseAnimation { duration: index*300 } NumberAnimation { from:0.3; to:1; duration:600 } NumberAnimation { from:1; to:0.3; duration:600 } }
                            }
                        }
                    }

                    Row {
                        anchors.centerIn: parent; visible: agingPhase === "testing"; spacing: 7
                        Repeater {
                            model: 4
                            Rectangle {
                                width: 6; height: 22; radius: 3; color: ["#4ade80","#22c55e","#22c55e","#16a34a"][index]
                                SequentialAnimation on height { loops: Animation.Infinite; PauseAnimation { duration: index*150 } PropertyAnimation { from:10; to:44; duration:400; easing.type: Easing.InOutQuad } PropertyAnimation { from:44; to:10; duration:400; easing.type: Easing.InOutQuad } }
                            }
                        }
                    }

                    Canvas {
                        anchors.centerIn: parent; visible: agingPhase === "finished"; width: 60; height: 60
                        property bool pass: agingResult === "PASS"
                        onPassChanged: requestPaint(); Component.onCompleted: requestPaint()
                        onPaint: { var c=getContext("2d"); c.clearRect(0,0,width,height); var cl=agingResult==="PASS"?"#4ade80":"#f87171"; c.strokeStyle=cl; c.lineWidth=5; c.lineCap="round"; c.lineJoin="round"; if(agingResult==="PASS"){c.beginPath();c.moveTo(8,32);c.lineTo(24,50);c.lineTo(52,14);c.stroke()}else{c.beginPath();c.moveTo(14,14);c.lineTo(46,46);c.stroke();c.beginPath();c.moveTo(46,14);c.lineTo(14,46);c.stroke()} }
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter; font.pixelSize: 18; font.bold: true
                    text: ({ "idle":"开始老化","waiting":"等待连接","testing":"老化中","finished":"老化完成" })[agingPhase] || ""
                    color: ({ "idle":"#94a3b8","waiting":"#fbbf24","testing":"#ffffff","finished":agingResult==="PASS"?"#4ade80":"#f87171" })[agingPhase] || "#94a3b8"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter; visible: agingPhase === "finished"
                    text: agingResult; font.pixelSize: 14; font.letterSpacing: 3
                    color: agingResult === "PASS" ? "#22c55e" : "#ef4444"
                }
            }

            MouseArea {
                anchors.fill: parent; activeFocusOnTab: false; acceptedButtons: Qt.LeftButton
                onClicked: {
                    if (agingPhase === "idle") { root.agingPhase="waiting"; root.agingResult=""; root.elapsedSec=0; agingTestManage.start() }
                    else { root.agingPhase="idle"; root.agingResult=""; agingTestManage.reset() }
                }
            }

            Rectangle {
                anchors.fill: parent; radius: width/2; color: "transparent"
                border { width: 8; color: ({ "idle":"#1e293b","waiting":"#f59e0b","testing":"#22c55e","finished":agingResult==="PASS"?"#22c55e":"#ef4444" })[agingPhase] || "#1e293b" }
                opacity: agingPhase === "idle" ? 1.0 : 0.5
            }
        }

        // 计时器
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 48; radius: 8; color: "#0f172a"; border { color: "#1e293b"; width: 1 }
            Text {
                anchors.centerIn: parent; text: formatTimer(elapsedSec)
                font.family: "Courier New"; font.pixelSize: 32; font.bold: true; color: "#60a5fa"
            }
        }

        // 操作按钮
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 8
                color: sMouse.containsMouse || sDrawer.visible ? "#0f2a1f" : "#1e293b"
                border { color: sMouse.containsMouse || sDrawer.visible ? "#34d399" : "#334155"; width: 1 }
                Text { anchors.centerIn: parent; text: "⚙ 老化设置"; font.pixelSize: 15; color: sMouse.containsMouse || sDrawer.visible ? "#34d399" : "#cbd5e1" }
                MouseArea { id: sMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: sDrawer.open() }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 8
                color: eMouse.containsMouse || eDrawer.visible ? "#0f2a1f" : "#1e293b"
                border { color: eMouse.containsMouse || eDrawer.visible ? "#34d399" : "#334155"; width: 1 }
                Text { anchors.centerIn: parent; text: "💾 导出结果"; font.pixelSize: 15; color: eMouse.containsMouse || eDrawer.visible ? "#34d399" : "#cbd5e1" }
                MouseArea { id: eMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: eDrawer.open() }
            }
        }
    }

    // 抽屉
    SlideDrawer { id: sDrawer; title: "老化设置"; drawerWidth: 1200; ColumnLayout { anchors.fill: parent; spacing: 16; Text { text:"老化参数配置"; font.pixelSize:16; font.weight:Font.Bold; color:"#e2e8f0" } Rectangle { Layout.fillWidth:true; height:1; color:"#1e293b" } Item { Layout.fillHeight:true } } }
    SlideDrawer { id: eDrawer; title: "导出结果"; drawerWidth: 1200; ColumnLayout { anchors.fill: parent; spacing: 16; Text { text:"数据导出"; font.pixelSize:16; font.weight:Font.Bold; color:"#e2e8f0" } Rectangle { Layout.fillWidth:true; height:1; color:"#1e293b" } Item { Layout.fillHeight:true } } }
}
