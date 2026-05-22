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
        var h = Math.floor(s / 3600); var m = Math.floor((s % 3600) / 60)
        return String(h).padStart(2,'0') + ":" + String(m).padStart(2,'0') + ":" + String(s % 60).padStart(2,'0')
    }

    Timer {
        interval: 1000; running: root.agingPhase === "testing"; repeat: true
        onTriggered: root.elapsedSec++
    }

    Rectangle {
        anchors.fill: parent; radius: 12
        border { color: "#1e293b"; width: 1 }
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0f172a" }
            GradientStop { position: 1.0; color: "#020617" }
        }
    }

    ColumnLayout {
        anchors { left: parent.left; right: parent.right; top: parent.top; bottom: parent.bottom
                  topMargin: 12; bottomMargin: 24; leftMargin: 24; rightMargin: 24 }
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: "老化测试控制台"
                font.family: "Inter, Segoe UI, sans-serif"; font.pixelSize: 20; font.weight: Font.Bold
                color: "#e2e8f0"
            }
            // 复位按钮
            Rectangle {
                implicitWidth: 80; implicitHeight: 32; radius: 6
                color: agingResetMouse.containsMouse ? "#0f2a1f" : "#1e293b"
                border { color: agingResetMouse.containsMouse ? "#34d399" : "#334155"; width: 1 }
                Row {
                    anchors.centerIn: parent; spacing: 6
                    Text { text: "\u21BA"; font.pixelSize: 18; font.weight: Font.Bold; color: agingResetMouse.containsMouse ? "#34d399" : "#cbd5e1" }
                    Text { text: "复位"; font.pixelSize: 13; color: agingResetMouse.containsMouse ? "#34d399" : "#cbd5e1" }
                }
                MouseArea {
                    id: agingResetMouse
                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        agingTestManage.reset()
                        root.agingPhase = "idle"
                        root.agingResult = ""
                        root.elapsedSec = 0
                    }
                }
            }
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
                    if (agingPhase === "testing" || agingPhase === "configuring") {
                        grad.addColorStop(0,"#22c55e"); grad.addColorStop(1,"#14532d")
                    } else if (agingPhase === "paused") {
                        grad.addColorStop(0,"#f59e0b"); grad.addColorStop(1,"#78350f")
                    } else if (agingPhase === "waiting") {
                        grad.addColorStop(0,"#f59e0b"); grad.addColorStop(1,"#78350f")
                    } else if (agingPhase === "finished") {
                        if (agingResult === "PASS") { grad.addColorStop(0,"#22c55e"); grad.addColorStop(1,"#064e3b") }
                        else { grad.addColorStop(0,"#ef4444"); grad.addColorStop(1,"#7f1d1d") }
                    } else {
                        grad.addColorStop(0,"#1e293b"); grad.addColorStop(1,"#0f172a")
                    }
                    ctx.beginPath(); ctx.arc(cx,cy,r-8,0,2*Math.PI); ctx.fillStyle = grad; ctx.fill()
                }
            }

            Column {
                anchors.centerIn: parent; spacing: 8
                Item {
                    anchors.horizontalCenter: parent.horizontalCenter; width: 55; height: 65

                    // idle
                    Canvas {
                        anchors.centerIn: parent; visible: agingPhase === "idle"; width: 55; height: 65
                        onPaint: {
                            var c = getContext("2d"); c.fillStyle = "#94a3b8"
                            c.beginPath(); c.moveTo(8,3); c.lineTo(width,height/2); c.lineTo(8,height-3)
                            c.closePath(); c.fill()
                        }
                        Component.onCompleted: requestPaint()
                    }

                    // waiting
                    Row {
                        anchors.centerIn: parent; visible: agingPhase === "waiting"; spacing: 12
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: "#fbbf24"
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.3; to: 1; duration: 600 }
                                NumberAnimation { from: 1; to: 0.3; duration: 600 }
                            }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: "#f59e0b"
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                PauseAnimation  { duration: 300 }
                                NumberAnimation { from: 0.3; to: 1; duration: 600 }
                                NumberAnimation { from: 1; to: 0.3; duration: 600 }
                            }
                        }
                        Rectangle {
                            width: 12; height: 12; radius: 6; color: "#d97706"
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                PauseAnimation  { duration: 600 }
                                NumberAnimation { from: 0.3; to: 1; duration: 600 }
                                NumberAnimation { from: 1; to: 0.3; duration: 600 }
                            }
                        }
                    }

                    // configuring
                    Text {
                        anchors.centerIn: parent; visible: agingPhase === "configuring"
                        text: "\u2699"; font.pixelSize: 40; color: "#22c55e"
                        RotationAnimator on rotation {
                            from: 0; to: 360; duration: 1500; loops: Animation.Infinite
                            running: agingPhase === "configuring"
                        }
                    }

                    // testing
                    Row {
                        anchors.centerIn: parent; visible: agingPhase === "testing"; spacing: 7
                        Rectangle {
                            width: 6; height: 22; radius: 3; color: "#4ade80"
                            SequentialAnimation on height {
                                loops: Animation.Infinite
                                PropertyAnimation { from: 10; to: 44; duration: 400; easing.type: Easing.InOutQuad }
                                PropertyAnimation { from: 44; to: 10; duration: 400; easing.type: Easing.InOutQuad }
                            }
                        }
                        Rectangle {
                            width: 6; height: 22; radius: 3; color: "#22c55e"
                            SequentialAnimation on height {
                                loops: Animation.Infinite
                                PauseAnimation  { duration: 150 }
                                PropertyAnimation { from: 10; to: 44; duration: 400; easing.type: Easing.InOutQuad }
                                PropertyAnimation { from: 44; to: 10; duration: 400; easing.type: Easing.InOutQuad }
                            }
                        }
                        Rectangle {
                            width: 6; height: 22; radius: 3; color: "#22c55e"
                            SequentialAnimation on height {
                                loops: Animation.Infinite
                                PauseAnimation  { duration: 300 }
                                PropertyAnimation { from: 10; to: 44; duration: 400; easing.type: Easing.InOutQuad }
                                PropertyAnimation { from: 44; to: 10; duration: 400; easing.type: Easing.InOutQuad }
                            }
                        }
                        Rectangle {
                            width: 6; height: 22; radius: 3; color: "#16a34a"
                            SequentialAnimation on height {
                                loops: Animation.Infinite
                                PauseAnimation  { duration: 450 }
                                PropertyAnimation { from: 10; to: 44; duration: 400; easing.type: Easing.InOutQuad }
                                PropertyAnimation { from: 44; to: 10; duration: 400; easing.type: Easing.InOutQuad }
                            }
                        }
                    }

                    // paused: 暂停双竖线
                    Row {
                        anchors.centerIn: parent; visible: agingPhase === "paused"; spacing: 10
                        Rectangle { width: 8; height: 36; radius: 2; color: "#fbbf24" }
                        Rectangle { width: 8; height: 36; radius: 2; color: "#f59e0b" }
                    }

                    // finished
                    Canvas {
                        anchors.centerIn: parent; visible: agingPhase === "finished"; width: 60; height: 60
                        property bool pass: agingResult === "PASS"
                        onPassChanged: requestPaint()
                        Component.onCompleted: requestPaint()
                        onPaint: {
                            var c = getContext("2d"); c.clearRect(0,0,width,height)
                            var cl = agingResult === "PASS" ? "#4ade80" : "#f87171"
                            c.strokeStyle = cl; c.lineWidth = 5; c.lineCap = "round"; c.lineJoin = "round"
                            if (agingResult === "PASS") {
                                c.beginPath(); c.moveTo(8,32); c.lineTo(24,50); c.lineTo(52,14); c.stroke()
                            } else {
                                c.beginPath(); c.moveTo(14,14); c.lineTo(46,46); c.stroke()
                                c.beginPath(); c.moveTo(46,14); c.lineTo(14,46); c.stroke()
                            }
                        }
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter; font.pixelSize: 18; font.bold: true
                    text: {
                        var map = { "idle":"开始老化","waiting":"等待连接","configuring":"下发配置","testing":"老化中","paused":"已暂停","finished":"老化完成" }
                        return map[agingPhase] || ""
                    }
                    color: {
                        if (agingPhase === "idle") return "#94a3b8"
                        if (agingPhase === "waiting" || agingPhase === "paused") return "#fbbf24"
                        if (agingPhase === "configuring" || agingPhase === "testing") return "#ffffff"
                        if (agingPhase === "finished") return agingResult === "PASS" ? "#4ade80" : "#f87171"
                        return "#94a3b8"
                    }
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
                    if (agingPhase === "idle") {
                        root.agingPhase = "waiting"; root.agingResult = ""; root.elapsedSec = 0
                        agingTestManage.start()
                    } else if (agingPhase === "testing") {
                        root.agingPhase = "paused"
                        agingTestManage.setPaused(true)
                    } else if (agingPhase === "paused") {
                        root.agingPhase = "testing"
                        agingTestManage.setPaused(false)
                    } else {
                        root.agingPhase = "idle"; root.agingResult = ""
                        agingTestManage.reset()
                    }
                }
            }

            Rectangle {
                anchors.fill: parent; radius: width/2; color: "transparent"
                border {
                    width: 8
                    color: {
                        if (agingPhase === "idle") return "#1e293b"
                        if (agingPhase === "waiting" || agingPhase === "paused") return "#f59e0b"
                        if (agingPhase === "configuring" || agingPhase === "testing") return "#22c55e"
                        if (agingPhase === "finished") return agingResult === "PASS" ? "#22c55e" : "#ef4444"
                        return "#1e293b"
                    }
                }
                opacity: agingPhase === "idle" ? 1.0 : 0.5
            }
        }

        // 计时器
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 48; radius: 8
            color: "#0f172a"; border { color: "#1e293b"; width: 1 }
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

    SlideDrawer { id: sDrawer; title: "老化设置"; drawerWidth: 1200; ColumnLayout { anchors.fill: parent; spacing: 16; Text { text:"老化参数配置"; font.pixelSize:16; font.weight:Font.Bold; color:"#e2e8f0" } Rectangle { Layout.fillWidth:true; height:1; color:"#1e293b" } Item { Layout.fillHeight:true } } }
    SlideDrawer { id: eDrawer; title: "导出结果"; drawerWidth: 1200; ColumnLayout { anchors.fill: parent; spacing: 16; Text { text:"数据导出"; font.pixelSize:16; font.weight:Font.Bold; color:"#e2e8f0" } Rectangle { Layout.fillWidth:true; height:1; color:"#1e293b" } Item { Layout.fillHeight:true } } }
}
