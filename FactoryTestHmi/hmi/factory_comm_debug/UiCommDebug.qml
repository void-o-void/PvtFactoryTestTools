import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"

    function appendSend(text) {
        sendModel.append({ text: text })
        if (sendModel.count > 200)
            sendModel.remove(0)
    }
    function appendRecv(text) {
        recvModel.append({ text: text })
        if (recvModel.count > 200)
            recvModel.remove(0)
    }

    ListModel { id: sendModel }
    ListModel { id: recvModel }

    Connections {
        target: commDebugManage
        function onSendJsonLog(prettyJson) { root.appendSend(prettyJson) }
        function onSendStatus(msg) { root.appendSend(msg) }
        function onRecvJsonLog(prettyJson) { root.appendRecv(prettyJson) }
        function onRecvStatus(msg) { root.appendRecv(msg) }
    }

    Component.onCompleted: {
        appendSend("[OK] 客户端发送区就绪\nJSON: " + commDebugManage.jsonPath)
        appendRecv("[OK] 服务器应答区就绪（物理 TX/RX）\n先启动再发送。\n通路：客户端 TX → 线缆/对端 → 服务器 RX(pull) → 组 ACK → TX 回传。\n无软件环回：需串口环回线、虚拟串口对，或对端设备回传。")
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

    RowLayout {
        anchors { fill: parent; margins: 20 }
        spacing: 16

        // ===== 左侧：上发送 / 下接收 =====
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ----- 发送区 -----
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 1
                radius: 10
                color: "#0b1220"
                border { color: "#1e293b"; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: 14 }
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "客户端 · 发送 JSON"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            color: "#c4b5fd"
                        }
                        Text {
                            text: "发送 " + commDebugManage.sendCount + " 次"
                            font.pixelSize: 12
                            color: "#a78bfa"
                        }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            id: sendBtn
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 34
                            radius: 8
                            color: sendMa.pressed ? "#5b21b6"
                                 : (sendMa.containsMouse ? "#4c1d95" : "#1e293b")
                            border {
                                width: 1
                                color: (sendMa.containsMouse || sendMa.pressed) ? "#a78bfa" : "#334155"
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "发送"
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: (sendMa.containsMouse || sendMa.pressed) ? "#c4b5fd" : "#e2e8f0"
                            }
                            MouseArea {
                                id: sendMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: commDebugManage.sendJson()
                            }
                        }
                    }

                    ListView {
                        id: sendView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: sendModel
                        spacing: 8
                        delegate: Text {
                            width: sendView.width
                            wrapMode: Text.Wrap
                            text: model.text
                            font.family: "Consolas, monospace"
                            font.pixelSize: 12
                            color: model.text.indexOf("[ERR]") === 0 ? "#f87171"
                                 : (model.text.indexOf("[WARN]") === 0 ? "#fbbf24" : "#cbd5e1")
                            lineHeight: 1.35
                        }
                        onCountChanged: positionViewAtEnd()
                    }
                }
            }

            // ----- 接收区 -----
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 1
                radius: 10
                color: "#0b1220"
                border { color: "#1e293b"; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: 14 }
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "服务器 · 收到请求 / 回传 ACK"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            color: "#67e8f9"
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: "回传 " + commDebugManage.ackCount + " 次"
                            font.pixelSize: 12
                            color: "#22d3ee"
                        }
                    }

                    ListView {
                        id: recvView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: recvModel
                        spacing: 8
                        delegate: Text {
                            width: recvView.width
                            wrapMode: Text.Wrap
                            text: model.text
                            font.family: "Consolas, monospace"
                            font.pixelSize: 12
                            color: model.text.indexOf("[ERR]") === 0 ? "#f87171"
                                 : (model.text.indexOf("[WARN]") === 0 ? "#fbbf24" : "#94a3b8")
                            lineHeight: 1.35
                        }
                        onCountChanged: positionViewAtEnd()
                    }
                }
            }
        }

        // ===== 右侧：启动串口接收 =====
        Rectangle {
            Layout.preferredWidth: 148
            Layout.fillHeight: true
            radius: 10
            color: "#0b1220"
            border { color: "#1e293b"; width: 1 }

            ColumnLayout {
                anchors { fill: parent; margins: 16 }
                spacing: 14

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: "服务器"
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: "#e2e8f0"
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: "监听并回 ACK"
                    font.pixelSize: 12
                    color: "#64748b"
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 10
                    radius: 5
                    color: "#1e293b"
                    Rectangle {
                        anchors.centerIn: parent
                        width: 10; height: 10; radius: 5
                        color: commDebugManage.serverListening ? "#34d399" : "#64748b"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: commDebugManage.serverListening ? "接收中" : "未启动"
                    font.pixelSize: 13
                    color: commDebugManage.serverListening ? "#34d399" : "#94a3b8"
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: "发 " + commDebugManage.sendCount
                          + " / 回 " + commDebugManage.ackCount
                    font.pixelSize: 12
                    color: "#94a3b8"
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: "清零计数"
                    font.pixelSize: 11
                    color: clearMa.containsMouse ? "#e2e8f0" : "#64748b"
                    MouseArea {
                        id: clearMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: commDebugManage.resetCounts()
                    }
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    id: startBtn
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    radius: 8
                    color: startMa.pressed ? "#065f46"
                         : (startMa.containsMouse ? "#0f2a1f" : "#1e293b")
                    border {
                        width: 1
                        color: (startMa.containsMouse || startMa.pressed || commDebugManage.serverListening)
                               ? "#34d399" : "#334155"
                    }
                    Text {
                        anchors.centerIn: parent
                        text: commDebugManage.serverListening ? "停止" : "启动"
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: (startMa.containsMouse || startMa.pressed || commDebugManage.serverListening)
                               ? "#34d399" : "#e2e8f0"
                    }
                    MouseArea {
                        id: startMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (commDebugManage.serverListening)
                                commDebugManage.stopServer()
                            else
                                commDebugManage.startServer()
                        }
                    }
                }
            }
        }
    }
}
