import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Rectangle {
    id: topHeader
    height: 96
    radius: 12
    antialiasing: true
    clip: true

    // 当前视图名称，供 Home 切换
    property string currentView: "功能测试"
    signal viewChanged(string viewName)

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#0f172a" }
        GradientStop { position: 1.0; color: "#020617" }
    }
    border.color: "#1e293b"
    border.width: 1

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "transparent"
        border.color: "#10ffffff"
        border.width: 1
    }

    // ----- 左侧：图标 + 标题 + 功能选择下拉 -----
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 24
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        // 工厂图标
        Image {
            width: 48; height: 48
            source: "qrc:/qt/qml/FactoryTestHmi/resources/pvt.png"
            fillMode: Image.PreserveAspectFit
            smooth: true
            anchors.verticalCenter: parent.verticalCenter
        }

        // 标题文字列
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Text {
                text: "SMARTTEST PRO"
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 24; font.weight: Font.Bold
                font.letterSpacing: 1.2
                color: "#60a5fa"
            }
            Row {
                spacing: 10
                Text {
                    text: "智能工厂测试集成系统"
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 12; font.letterSpacing: 1.2
                    color: "#94a3b8"
                }
                Text {
                    text: "V2.4.0"
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 14; font.weight: Font.Light
                    color: "#64748b"
                }
            }
        }

        // 分隔
        Rectangle {
            width: 1; height: 36
            color: "#1e293b"
            anchors.verticalCenter: parent.verticalCenter
        }

        // 功能选择下拉按钮
        Rectangle {
            id: navButton
            width: 160; height: 40
            radius: 6
            color: navMouse.containsMouse || navPopup.visible ? "#1e293b" : "transparent"
            border {
                color: navPopup.visible ? "#60a5fa" : (navMouse.containsMouse ? "#334155" : "#1e293b")
                width: 1
            }

            Behavior on color { ColorAnimation { duration: 150 } }

            Row {
                anchors.centerIn: parent
                spacing: 8

                // 当前视图指示点
                Rectangle {
                    width: 8; height: 8; radius: 4
                    color: topHeader.currentView === "功能测试" ? "#60a5fa" : "#f59e0b"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: topHeader.currentView
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 14; font.weight: Font.Medium
                    color: "#e2e8f0"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: "\u25BC"
                    font.pixelSize: 16; color: "#22c55e"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: navMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: navPopup.visible ? navPopup.close() : navPopup.open()
            }

            // 下拉菜单（在 navButton 内部，自然对齐下方）
            Popup {
                id: navPopup
                y: navButton.height + 4
                width: 200
                padding: 4
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                background: Rectangle {
                    radius: 10
                    color: "#0f172a"
                    border { color: "#1e293b"; width: 1 }
                }

                contentItem: Column {
                    spacing: 2

                    Rectangle {
                        width: parent.width; height: 42; radius: 6
                        color: topHeader.currentView === "功能测试" ? "#1e3a5f" :
                               (featureMouse.containsMouse ? "#1e293b" : "transparent")
                        Behavior on color { ColorAnimation { duration: 120 } }

                        Row {
                            anchors.centerIn: parent; spacing: 10
                            Rectangle {
                                width: 6; height: 6; radius: 3
                                color: topHeader.currentView === "功能测试" ? "#60a5fa" : "#475569"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "功能测试"
                                font.pixelSize: 14
                                color: topHeader.currentView === "功能测试" ? "#60a5fa" : "#94a3b8"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            id: featureMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                topHeader.currentView = "功能测试"
                                topHeader.viewChanged("功能测试")
                                testManage.reset()
                                navPopup.close()
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width; height: 42; radius: 6
                        color: topHeader.currentView === "老化测试" ? "#1e3a5f" :
                               (agingMouse.containsMouse ? "#1e293b" : "transparent")
                        Behavior on color { ColorAnimation { duration: 120 } }

                        Row {
                            anchors.centerIn: parent; spacing: 10
                            Rectangle {
                                width: 6; height: 6; radius: 3
                                color: topHeader.currentView === "老化测试" ? "#f59e0b" : "#475569"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "老化测试"
                                font.pixelSize: 14
                                color: topHeader.currentView === "老化测试" ? "#f59e0b" : "#94a3b8"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            id: agingMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                topHeader.currentView = "老化测试"
                                topHeader.viewChanged("老化测试")
                                navPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    // ----- 右侧：时间 + 状态 -----
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 32
        anchors.verticalCenter: parent.verticalCenter
        spacing: 32

        Column {
            spacing: 6
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: clockText
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 20; font.weight: Font.Bold
                color: "#cbd5e1"
                text: "2026-04-22 10:45:30"

                Timer {
                    interval: 1000; running: true; repeat: true
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

            Row {
                spacing: 8
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    width: 8; height: 8; radius: 4; color: "#22c55e"
                    anchors.verticalCenter: parent.verticalCenter
                    SequentialAnimation on opacity {
                        running: true; loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                        NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                    }
                }
                Text {
                    text: "系统运行正常"
                    font.family: "Inter, Segoe UI, sans-serif"
                    font.pixelSize: 12; color: "#22c55e"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

}
