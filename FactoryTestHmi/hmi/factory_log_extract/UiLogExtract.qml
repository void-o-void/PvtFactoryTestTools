import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "transparent"

    property string phase: "idle"   // idle / extracting / done / fail / nodevice
    // 各系统自定义保存路径（空则用默认 FactoryLogs/系统名）
    property var customSavePaths: ({ "YOCTO": "", "TBOX": "", "Android": "" })

    function systemPathOf(title) {
        var p = customSavePaths[title]
        if (p && p.length > 0)
            return p
        return logExtractManage.localRoot + "/" + title
    }

    function pickPathFor(title) {
        var start = systemPathOf(title)
        var chosen = logExtractManage.pickDirectory(start)
        if (!chosen || chosen.length === 0)
            return
        var next = {
            "YOCTO": customSavePaths["YOCTO"] || "",
            "TBOX": customSavePaths["TBOX"] || "",
            "Android": customSavePaths["Android"] || ""
        }
        next[title] = chosen
        customSavePaths = next
        appendOp("[OK] " + title + " 保存路径已设为: " + chosen)
    }

    function extractOne(title) {
        if (root.phase === "extracting")
            return
        root.phase = "extracting"
        var dir = customSavePaths[title] || ""
        logExtractManage.extractSystem(title, dir)
    }

    function appendOp(line) {
        opLogModel.append({ text: line })
        if (opLogModel.count > 500)
            opLogModel.remove(0)
    }

    ListModel { id: opLogModel }
    ListModel { id: fileModel }

    function refreshLocalFiles() {
        fileModel.clear()
        // 仅展示提示；真实文件列表由操作日志反馈
    }

    Connections {
        target: logExtractManage
        function onLogMessage(msg) { appendOp(msg) }
        function onNoDeviceConnected() {
            root.phase = "nodevice"
            appendOp("[ERR] 无设备连接")
        }
        function onExtractingChanged() {
            if (logExtractManage.extracting)
                root.phase = "extracting"
        }
        function onExtractFinished(ok, summary) {
            root.phase = ok ? "done" : (root.phase === "nodevice" ? "nodevice" : "fail")
            appendOp(ok ? ("[OK] " + summary) : ("[ERR] " + summary))
        }
        function onLocalCleared(ok, msg) {
            appendOp(ok ? ("[OK] " + msg) : ("[ERR] " + msg))
            if (ok && root.phase !== "extracting")
                root.phase = "idle"
        }
    }

    Component.onCompleted: {
        appendOp("[OK] 日志提取就绪")
        appendOp("[OK] 本地目录: " + logExtractManage.localRoot)
        appendOp("[OK] 系统: YOCTO / TBOX / Android  ← 型号见 config/logcfg.json")
    }

    // ===== 左侧控制台 =====
    Rectangle {
        id: controlCard
        width: 480; height: 550
        radius: 12
        color: "transparent"

        Rectangle {
            anchors.fill: parent; radius: 12
            border { color: "#1e293b"; width: 1 }
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0f172a" }
                GradientStop { position: 1.0; color: "#020617" }
            }
        }

        ColumnLayout {
            anchors {
                fill: parent
                margins: 24
                topMargin: 16; bottomMargin: 24
            }
            spacing: 14

            Text {
                text: "日志提取控制台"
                font.family: "Inter, Segoe UI, sans-serif"
                font.pixelSize: 20; font.weight: Font.Bold
                color: "#e2e8f0"
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e293b" }

            // 状态指示
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                radius: 10
                color: "#0b1220"
                border {
                    width: 1
                    color: root.phase === "nodevice" ? "#7f1d1d"
                         : root.phase === "extracting" ? "#155e75"
                         : root.phase === "done" ? "#14532d"
                         : root.phase === "fail" ? "#7f1d1d" : "#1e293b"
                }

                Row {
                    anchors.centerIn: parent; spacing: 12
                    Rectangle {
                        width: 10; height: 10; radius: 5
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.phase === "extracting" ? "#22d3ee"
                             : root.phase === "done" ? "#34d399"
                             : root.phase === "nodevice" || root.phase === "fail" ? "#f87171"
                             : "#64748b"
                        SequentialAnimation on opacity {
                            running: root.phase === "extracting"
                            loops: Animation.Infinite
                            NumberAnimation { from: 1; to: 0.35; duration: 700 }
                            NumberAnimation { from: 0.35; to: 1; duration: 700 }
                        }
                    }
                    Column {
                        spacing: 4
                        Text {
                            text: root.phase === "nodevice" ? "无设备连接"
                                : root.phase === "extracting" ? "正在提取全部日志…"
                                : root.phase === "done" ? "提取完成"
                                : root.phase === "fail" ? "提取失败 / 部分失败"
                                : "待机 · 等待提取"
                            font.pixelSize: 16; font.weight: Font.Medium; color: "#e2e8f0"
                        }
                        Text {
                            text: logExtractManage.statusText
                            font.pixelSize: 12; color: "#64748b"
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: 13; color: "#94a3b8"
                text: "将通过 ADB 进入系统，拉取 /data/debuglogger/mobilelog，并按 YOCTO / TBOX / Android 分别保存到本地默认目录。"
            }

            Text {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                font.pixelSize: 12; color: "#64748b"
                text: "保存路径: " + logExtractManage.localRoot
            }

            Item { Layout.fillHeight: true }

            // 提取所有日志
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 8
                color: extractMouse.containsMouse ? "#083344" : "#0b1220"
                border {
                    width: 1
                    color: root.phase === "extracting" ? "#334155"
                         : (extractMouse.containsMouse ? "#22d3ee" : "#1e293b")
                }
                opacity: root.phase === "extracting" ? 0.55 : 1.0

                Row {
                    anchors.centerIn: parent; spacing: 10
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#22d3ee"
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.phase === "extracting" ? "提取中…" : "提取所有日志"
                        font.pixelSize: 16; font.weight: Font.Medium
                        color: "#e2e8f0"
                    }
                }

                MouseArea {
                    id: extractMouse
                    anchors.fill: parent
                    hoverEnabled: root.phase !== "extracting"
                    cursorShape: root.phase === "extracting" ? Qt.ArrowCursor : Qt.PointingHandCursor
                    enabled: root.phase !== "extracting"
                    onClicked: {
                        root.phase = "extracting"
                        logExtractManage.extractAll()
                    }
                }
            }

            // 清空本地日志
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 8
                color: clearMouse.containsMouse ? "#3f1d1d" : "#0b1220"
                border {
                    width: 1
                    color: clearMouse.containsMouse ? "#f87171" : "#1e293b"
                }
                opacity: root.phase === "extracting" ? 0.45 : 1.0

                Row {
                    anchors.centerIn: parent; spacing: 10
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#f87171"
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "清空本地日志"
                        font.pixelSize: 16; font.weight: Font.Medium
                        color: "#e2e8f0"
                    }
                }

                MouseArea {
                    id: clearMouse
                    anchors.fill: parent
                    hoverEnabled: root.phase !== "extracting"
                    enabled: root.phase !== "extracting"
                    cursorShape: root.phase === "extracting" ? Qt.ArrowCursor : Qt.PointingHandCursor
                    onClicked: confirmClear.open()
                }
            }
        }
    }

    Popup {
        id: confirmClear
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 420; height: 180
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            radius: 12; color: "#0f172a"; border { color: "#1e293b"; width: 1 }
        }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 16
            Text {
                text: "确认清空本地日志？"
                font.pixelSize: 18; font.weight: Font.Bold; color: "#e2e8f0"
            }
            Text {
                Layout.fillWidth: true; wrapMode: Text.Wrap
                text: "将删除 FactoryLogs 下 YOCTO / TBOX / Android 中的已提取文件，此操作不可恢复。"
                font.pixelSize: 13; color: "#94a3b8"
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                Item { Layout.fillWidth: true }
                Rectangle {
                    implicitWidth: 88; implicitHeight: 36; radius: 6
                    color: "#1e293b"; border { color: "#334155"; width: 1 }
                    Text { anchors.centerIn: parent; text: "取消"; color: "#cbd5e1"; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: confirmClear.close() }
                }
                Rectangle {
                    implicitWidth: 88; implicitHeight: 36; radius: 6
                    color: "#7f1d1d"; border { color: "#f87171"; width: 1 }
                    Text { anchors.centerIn: parent; text: "清空"; color: "#fecaca"; font.pixelSize: 14 }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            confirmClear.close()
                            logExtractManage.clearLocalLogs()
                        }
                    }
                }
            }
        }
    }

    // ===== 左下操作日志 =====
    Rectangle {
        id: opLogCard
        anchors.top: controlCard.bottom
        anchors.topMargin: 16
        anchors.left: controlCard.left
        anchors.bottom: parent.bottom
        width: 480
        radius: 12
        color: "transparent"

        Rectangle {
            anchors.fill: parent; radius: 12
            border { color: "#1e293b"; width: 1 }
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0f172a" }
                GradientStop { position: 1.0; color: "#020617" }
            }
        }

        ColumnLayout {
            anchors { fill: parent; margins: 16 }
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Text { text: "操作日志"; font.pixelSize: 16; font.weight: Font.Bold; color: "#94a3b8" }
                Item { Layout.fillWidth: true }
                Text { text: opLogModel.count + " 条"; font.pixelSize: 12; color: "#64748b" }
                Rectangle {
                    Layout.preferredWidth: 60; Layout.preferredHeight: 28; radius: 4
                    color: clearLogMouse.containsMouse ? "#1e293b" : "transparent"
                    border { color: "#334155"; width: 1 }
                    Text {
                        anchors.centerIn: parent; text: "清空"; font.pixelSize: 12
                        color: clearLogMouse.containsMouse ? "#e2e8f0" : "#94a3b8"
                    }
                    MouseArea {
                        id: clearLogMouse
                        anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: opLogModel.clear()
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e293b" }

            ListView {
                id: opLogView
                Layout.fillWidth: true; Layout.fillHeight: true
                model: opLogModel
                clip: true
                spacing: 2
                onCountChanged: Qt.callLater(function() { positionViewAtEnd() })
                ScrollBar.vertical: ScrollBar { width: 4; policy: ScrollBar.AsNeeded }

                delegate: Text {
                    width: opLogView.width - 8
                    text: model.text
                    font.family: "Consolas, monospace"; font.pixelSize: 12
                    color: {
                        if (model.text.indexOf("[ERR]") >= 0) return "#f87171"
                        if (model.text.indexOf("[WARN]") >= 0) return "#fbbf24"
                        if (model.text.indexOf("[OK]") >= 0) return "#4ade80"
                        return "#94a3b8"
                    }
                    wrapMode: Text.WrapAnywhere
                    lineHeight: 1.45
                }
            }
        }
    }

    // // ===== 站位信息 =====
    // Rectangle {
    //     id: stationInfo
    //     anchors.top: controlCard.top
    //     anchors.left: controlCard.right; anchors.leftMargin: 16
    //     width: 1392; height: 50; color: "#0f172a"; radius: 10; antialiasing: true

    //     readonly property real itemWidth: (width - 40 - 4) / 5

    //     component StationItem : Item {
    //         property alias label: labelText.text
    //         property alias value: valueText.text
    //         Row {
    //             spacing: 8; height: parent.height; anchors.centerIn: parent
    //             Text {
    //                 id: labelText; height: parent.height
    //                 verticalAlignment: Text.AlignVCenter
    //                 font.pixelSize: 14; color: "#64748b"
    //             }
    //             Text {
    //                 id: valueText; height: parent.height
    //                 verticalAlignment: Text.AlignVCenter
    //                 font.pixelSize: 17; font.weight: Font.Medium; color: "#e2e8f0"
    //             }
    //         }
    //     }

    //     StationItem {
    //         id: s1
    //         anchors.left: parent.left; anchors.leftMargin: 20
    //         anchors.verticalCenter: parent.verticalCenter
    //         width: stationInfo.itemWidth; height: parent.height
    //         label: "工单号"; value: "WO-20260510-0038"
    //     }
    //     Rectangle { anchors.left: s1.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
    //     StationItem {
    //         id: s2
    //         anchors.left: s1.right; anchors.leftMargin: 1
    //         anchors.verticalCenter: parent.verticalCenter
    //         width: stationInfo.itemWidth; height: parent.height
    //         label: "工序名称"; value: "日志提取"
    //     }
    //     Rectangle { anchors.left: s2.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
    //     StationItem {
    //         id: s3
    //         anchors.left: s2.right; anchors.leftMargin: 1
    //         anchors.verticalCenter: parent.verticalCenter
    //         width: stationInfo.itemWidth; height: parent.height
    //         label: "员工工号"; value: "EMP8823"
    //     }
    //     Rectangle { anchors.left: s3.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
    //     StationItem {
    //         id: s4
    //         anchors.left: s3.right; anchors.leftMargin: 1
    //         anchors.verticalCenter: parent.verticalCenter
    //         width: stationInfo.itemWidth; height: parent.height
    //         label: "线别"; value: "L3-2"
    //     }
    //     Rectangle { anchors.left: s4.right; anchors.verticalCenter: parent.verticalCenter; width: 1; height: 28; color: "#1e293b" }
    //     StationItem {
    //         id: s5
    //         anchors.left: s4.right; anchors.leftMargin: 1
    //         anchors.verticalCenter: parent.verticalCenter
    //         width: stationInfo.itemWidth; height: parent.height
    //         label: "夹具编号"; value: "FIX-017"
    //     }
    // }

    // ===== 右侧说明 / 配置预览 =====
    Rectangle {
        anchors.top: controlCard.top
        anchors.topMargin: 0
        anchors.left: controlCard.right; anchors.leftMargin: 16
        anchors.bottom: parent.bottom
        width: 1392
        radius: 12
        color: "transparent"

        Rectangle {
            anchors.fill: parent; radius: 12
            border { color: "#1e293b"; width: 1 }
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0f172a" }
                GradientStop { position: 1.0; color: "#020617" }
            }
        }

        ColumnLayout {
            anchors { fill: parent; margins: 24 }
            spacing: 16

            Text {
                text: "提取说明"
                font.pixelSize: 18; font.weight: Font.Bold; color: "#e2e8f0"
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e293b" }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 16
                rowSpacing: 16

                Repeater {
                    model: [
                        { title: "YOCTO",  accent: "#22c55e", tip: "型号见 logcfg → systems.YOCTO", tag: "mobilelog-Y" },
                        { title: "TBOX",   accent: "#f59e0b", tip: "默认 127.0.0.1:7667", tag: "mobilelog-T" },
                        { title: "Android", accent: "#60a5fa", tip: "默认 127.0.0.1:7666", tag: "mobilelog-A" }
                    ]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        radius: 10
                        color: "#0b1220"
                        border { color: "#1e293b"; width: 1 }

                        Column {
                            anchors.fill: parent; anchors.margins: 14; spacing: 8
                            Row {
                                spacing: 8
                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: modelData.accent
                                }
                                Text {
                                    text: modelData.title
                                    font.pixelSize: 16; font.weight: Font.Bold; color: "#e2e8f0"
                                }
                            }
                            Text {
                                width: parent.width
                                wrapMode: Text.Wrap
                                text: modelData.tip
                                font.pixelSize: 12; color: "#94a3b8"
                            }
                            Text {
                                width: parent.width
                                wrapMode: Text.Wrap
                                elide: Text.ElideMiddle
                                text: "保存到: " + root.systemPathOf(modelData.title)
                                font.pixelSize: 11; color: "#64748b"
                            }
                            Text {
                                width: parent.width
                                text: "目录名: " + modelData.tag + " / " + modelData.tag + "(1)…"
                                font.pixelSize: 11; color: "#475569"
                            }
                            Row {
                                spacing: 8
                                width: parent.width
                                Rectangle {
                                    width: (parent.width - 8) / 2
                                    height: 34
                                    radius: 6
                                    color: extractOneMouse.containsMouse ? "#083344" : "#111827"
                                    border {
                                        width: 1
                                        color: root.phase === "extracting" ? "#334155"
                                             : (extractOneMouse.containsMouse ? modelData.accent : "#1e293b")
                                    }
                                    opacity: root.phase === "extracting" ? 0.5 : 1.0
                                    Text {
                                        anchors.centerIn: parent
                                        text: "提取日志"
                                        font.pixelSize: 13; font.weight: Font.Medium; color: "#e2e8f0"
                                    }
                                    MouseArea {
                                        id: extractOneMouse
                                        anchors.fill: parent
                                        hoverEnabled: root.phase !== "extracting"
                                        enabled: root.phase !== "extracting"
                                        cursorShape: root.phase === "extracting" ? Qt.ArrowCursor : Qt.PointingHandCursor
                                        onClicked: root.extractOne(modelData.title)
                                    }
                                }
                                Rectangle {
                                    width: (parent.width - 8) / 2
                                    height: 34
                                    radius: 6
                                    color: pathMouse.containsMouse ? "#1e293b" : "#111827"
                                    border {
                                        width: 1
                                        color: root.phase === "extracting" ? "#334155"
                                             : (pathMouse.containsMouse ? "#94a3b8" : "#1e293b")
                                    }
                                    opacity: root.phase === "extracting" ? 0.5 : 1.0
                                    Text {
                                        anchors.centerIn: parent
                                        text: "路径选择"
                                        font.pixelSize: 13; font.weight: Font.Medium; color: "#cbd5e1"
                                    }
                                    MouseArea {
                                        id: pathMouse
                                        anchors.fill: parent
                                        hoverEnabled: root.phase !== "extracting"
                                        enabled: root.phase !== "extracting"
                                        cursorShape: root.phase === "extracting" ? Qt.ArrowCursor : Qt.PointingHandCursor
                                        onClicked: root.pickPathFor(modelData.title)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#0b1220"
                border { color: "#1e293b"; width: 1 }

                ColumnLayout {
                    anchors { fill: parent; margins: 20 }
                    spacing: 12
                    Text {
                        text: "ADB 流程（代码自动执行）"
                        font.pixelSize: 15; font.weight: Font.Medium; color: "#94a3b8"
                    }
                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        font.family: "Consolas, monospace"
                        font.pixelSize: 13
                        color: "#64748b"
                        lineHeight: 1.55
                        text: "1) adb devices                     ← 无设备则界面提示「无设备连接」\n"
                            + "2) adb root\n"
                            + "3) adb -d forward tcp:<port> tcp:6666\n"
                            + "4) adb connect 127.0.0.1:<port>\n"
                            + "5) adb -s <serial> pull /data/debuglogger/mobilelog <本地目录>\n"
                            + "6) 目录名: YOCTO→mobilelog-Y  TBOX→mobilelog-T  Android→mobilelog-A\n"
                            + "   重复则 mobilelog-Y(1) / mobilelog-T(1)…（ASCII 括号）\n\n"
                            + "可对单系统点「提取日志」；「路径选择」可自定义该系统保存目录。"
                    }

                    // 无设备时醒目提示条
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 8
                        visible: root.phase === "nodevice"
                        color: "#450a0a"
                        border { color: "#f87171"; width: 1 }
                        Text {
                            anchors.centerIn: parent
                            text: "无设备连接 — 请检查 USB / ADB 后再点击「提取所有日志」"
                            font.pixelSize: 14; font.weight: Font.Medium; color: "#fecaca"
                        }
                    }
                }
            }
        }
    }
}
