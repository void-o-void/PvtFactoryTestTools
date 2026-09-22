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

    // 各系统连接状态（绿=可拉取，红=不可拉取）
    property bool yoctoOnline: logExtractManage.yoctoOnline
    property bool tboxOnline: logExtractManage.tboxOnline
    property bool androidOnline: logExtractManage.androidOnline
    property bool refreshingConn: false

    function systemOnlineOf(title) {
        if (title === "YOCTO") return root.yoctoOnline
        if (title === "TBOX") return root.tboxOnline
        if (title === "Android") return root.androidOnline
        return false
    }

    function refreshConnections() {
        if (root.phase === "extracting" || root.refreshingConn)
            return
        root.refreshingConn = true
        logExtractManage.refreshConnectionStatus()
    }

    function appendOp(line) {
        opLogModel.append({ text: line })
        if (opLogModel.count > 500)
            opLogModel.remove(0)
    }

    ListModel { id: opLogModel }
    ListModel { id: fileModel }

    readonly property string iconExtractAll: "qrc:/qt/qml/FactoryTestHmi/resources/ic_log_extract_all.png"
    readonly property string iconClear: "qrc:/qt/qml/FactoryTestHmi/resources/ic_log_clear.png"
    readonly property string iconExtract: "qrc:/qt/qml/FactoryTestHmi/resources/ic_log_extract.png"
    readonly property string iconFolder: "qrc:/qt/qml/FactoryTestHmi/resources/ic_log_folder.png"

    // 与功能测试 / 老化测试同风格：全部按钮悬停都是绿色
    component ActionBtn : Rectangle {
        id: btn
        property string label: ""
        property string iconSource: ""
        property bool busy: false
        property int iconSize: 22
        property int fontSize: 16
        signal clicked()

        readonly property color accent: "#34d399"
        readonly property color accentBg: "#0f2a1f"
        readonly property color pressBg: "#064e3b"
        readonly property bool hot: ma.containsMouse || ma.pressed

        Layout.fillWidth: true
        Layout.preferredHeight: 52
        radius: 8
        opacity: busy ? 0.55 : 1.0
        scale: ma.pressed && !busy ? 0.98 : 1.0
        color: busy ? "#1e293b"
             : (ma.pressed ? pressBg : (ma.containsMouse ? accentBg : "#1e293b"))
        border {
            width: 1
            color: busy ? "#334155" : (btn.hot ? accent : "#334155")
        }

        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
        Behavior on scale { NumberAnimation { duration: 80 } }

        Row {
            anchors.centerIn: parent
            spacing: 10
            Image {
                width: btn.iconSize; height: btn.iconSize
                anchors.verticalCenter: parent.verticalCenter
                source: btn.iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
                opacity: btn.busy ? 0.7 : 1.0
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: btn.label
                font.pixelSize: btn.fontSize
                font.weight: Font.Medium
                color: btn.busy ? "#64748b" : (btn.hot ? btn.accent : "#cbd5e1")
                Behavior on color { ColorAnimation { duration: 120 } }
            }
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: !btn.busy
            enabled: !btn.busy
            cursorShape: btn.busy ? Qt.ArrowCursor : Qt.PointingHandCursor
            onClicked: btn.clicked()
        }
    }

    component MiniActionBtn : Rectangle {
        id: mbtn
        property string label: ""
        property string iconSource: ""
        property bool busy: false
        signal clicked()

        readonly property color accent: "#34d399"
        readonly property color accentBg: "#0f2a1f"
        readonly property color pressBg: "#064e3b"
        readonly property bool hot: mma.containsMouse || mma.pressed

        height: 36
        radius: 8
        opacity: busy ? 0.5 : 1.0
        scale: mma.pressed && !busy ? 0.97 : 1.0
        color: busy ? "#1e293b"
             : (mma.pressed ? pressBg : (mma.containsMouse ? accentBg : "#1e293b"))
        border {
            width: 1
            color: busy ? "#334155" : (mbtn.hot ? accent : "#334155")
        }
        Behavior on color { ColorAnimation { duration: 120 } }
        Behavior on border.color { ColorAnimation { duration: 120 } }
        Behavior on scale { NumberAnimation { duration: 80 } }

        Row {
            anchors.centerIn: parent
            spacing: 6
            Image {
                width: 16; height: 16
                anchors.verticalCenter: parent.verticalCenter
                source: mbtn.iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: mbtn.label
                font.pixelSize: 13; font.weight: Font.Medium
                color: mbtn.busy ? "#64748b" : (mbtn.hot ? mbtn.accent : "#cbd5e1")
                Behavior on color { ColorAnimation { duration: 120 } }
            }
        }

        MouseArea {
            id: mma
            anchors.fill: parent
            hoverEnabled: !mbtn.busy
            enabled: !mbtn.busy
            cursorShape: mbtn.busy ? Qt.ArrowCursor : Qt.PointingHandCursor
            onClicked: mbtn.clicked()
        }
    }

    function refreshLocalFiles() {
        fileModel.clear()
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
            root.refreshConnections()
        }
        function onLocalCleared(ok, msg) {
            appendOp(ok ? ("[OK] " + msg) : ("[ERR] " + msg))
            if (ok && root.phase !== "extracting")
                root.phase = "idle"
        }
        function onConnectionStatusChanged() {
            root.yoctoOnline = logExtractManage.yoctoOnline
            root.tboxOnline = logExtractManage.tboxOnline
            root.androidOnline = logExtractManage.androidOnline
        }
        function onConnectionRefreshFinished() {
            root.refreshingConn = false
        }
        function onActivePlatformChanged() {
            var ids = logExtractManage.platformIds
            var cur = logExtractManage.activePlatformId
            var idx = ids.indexOf(cur)
            if (idx >= 0 && platformCombo.currentIndex !== idx)
                platformCombo.currentIndex = idx
        }
        function onPlatformsChanged() {
            platformCombo.model = logExtractManage.platformNames
            var ids = logExtractManage.platformIds
            var cur = logExtractManage.activePlatformId
            var idx = ids.indexOf(cur)
            if (idx >= 0)
                platformCombo.currentIndex = idx
        }
    }

    Component.onCompleted: {
        appendOp("[OK] 日志提取就绪")
        appendOp("[OK] 本地目录: " + logExtractManage.localRoot)
        appendOp("[OK] 平台: " + logExtractManage.activePlatformId
                 + " ｜ 系统: YOCTO / TBOX / Android")
        root.refreshConnections()
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

            // 状态指示：拉取中绿闪 / 完成常亮绿 / 失败或无设备红
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                radius: 10
                color: "#0b1220"
                border {
                    width: 1
                    color: root.phase === "extracting" || root.phase === "done" ? "#14532d"
                         : root.phase === "nodevice" || root.phase === "fail" ? "#7f1d1d"
                         : "#1e293b"
                }

                Row {
                    anchors.centerIn: parent; spacing: 12
                    Rectangle {
                        id: statusDot
                        width: 10; height: 10; radius: 5
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.phase === "extracting" || root.phase === "done" ? "#34d399"
                             : root.phase === "nodevice" || root.phase === "fail" ? "#f87171"
                             : "#64748b"
                        SequentialAnimation on opacity {
                            running: root.phase === "extracting"
                            loops: Animation.Infinite
                            NumberAnimation { from: 1; to: 0.25; duration: 550 }
                            NumberAnimation { from: 0.25; to: 1; duration: 550 }
                        }
                        onVisibleChanged: if (root.phase !== "extracting") opacity = 1
                    }
                    // 非闪烁时强制常亮
                    Binding {
                        target: statusDot
                        property: "opacity"
                        value: 1
                        when: root.phase !== "extracting"
                    }
                    Column {
                        spacing: 4
                        Text {
                            text: root.phase === "nodevice" ? "无设备连接"
                                : root.phase === "extracting" ? "正在提取日志…"
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
                text: "先选择平台，再按 USB_SWITCH 决定直连/转发；拉取 remote_path（默认 /data/debuglogger/mobilelog），按 YOCTO / TBOX / Android 分别保存。"
            }

            Text {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                font.pixelSize: 12; color: "#64748b"
                text: "保存路径: " + logExtractManage.localRoot
            }

            Item { Layout.fillHeight: true }

            ActionBtn {
                label: root.phase === "extracting" ? "提取中…" : "提取所有日志"
                iconSource: root.iconExtractAll
                busy: root.phase === "extracting"
                onClicked: {
                    root.phase = "extracting"
                    logExtractManage.extractAll()
                }
            }

            ActionBtn {
                label: "清空本地日志"
                iconSource: root.iconClear
                busy: root.phase === "extracting"
                onClicked: confirmClear.open()
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
                    id: cancelBtn
                    implicitWidth: 96; implicitHeight: 36; radius: 8
                    scale: cancelMa.pressed ? 0.97 : 1.0
                    color: cancelMa.pressed ? "#0f172a"
                         : (cancelMa.containsMouse ? "#0f2a1f" : "#1e293b")
                    border {
                        width: 1
                        color: (cancelMa.containsMouse || cancelMa.pressed) ? "#34d399" : "#334155"
                    }
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }
                    Text {
                        anchors.centerIn: parent; text: "取消"; font.pixelSize: 14
                        color: (cancelMa.containsMouse || cancelMa.pressed) ? "#34d399" : "#cbd5e1"
                    }
                    MouseArea {
                        id: cancelMa
                        anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: confirmClear.close()
                    }
                }
                Rectangle {
                    id: confirmBtn
                    implicitWidth: 96; implicitHeight: 36; radius: 8
                    scale: confirmMa.pressed ? 0.97 : 1.0
                    color: confirmMa.pressed ? "#064e3b"
                         : (confirmMa.containsMouse ? "#0f2a1f" : "#1e293b")
                    border {
                        width: 1
                        color: (confirmMa.containsMouse || confirmMa.pressed) ? "#34d399" : "#334155"
                    }
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }
                    Row {
                        anchors.centerIn: parent; spacing: 6
                        Image {
                            width: 14; height: 14
                            anchors.verticalCenter: parent.verticalCenter
                            source: root.iconClear
                            fillMode: Image.PreserveAspectFit
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "清空"; font.pixelSize: 14
                            color: (confirmMa.containsMouse || confirmMa.pressed) ? "#34d399" : "#cbd5e1"
                        }
                    }
                    MouseArea {
                        id: confirmMa
                        anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
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
                    id: clearLogBtn
                    Layout.preferredWidth: 72; Layout.preferredHeight: 28; radius: 6
                    scale: clearLogMouse.pressed ? 0.96 : 1.0
                    color: clearLogMouse.pressed ? "#0f172a"
                         : (clearLogMouse.containsMouse ? "#0f2a1f" : "transparent")
                    border {
                        width: 1
                        color: (clearLogMouse.containsMouse || clearLogMouse.pressed) ? "#34d399" : "#334155"
                    }
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }
                    Text {
                        anchors.centerIn: parent; text: "清空"; font.pixelSize: 12
                        color: (clearLogMouse.containsMouse || clearLogMouse.pressed) ? "#34d399" : "#94a3b8"
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

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: "提取说明"
                    font.pixelSize: 18; font.weight: Font.Bold; color: "#e2e8f0"
                }
                Item { Layout.fillWidth: true }

                Text {
                    text: "平台选择"
                    font.pixelSize: 14
                    color: "#94a3b8"
                    Layout.alignment: Qt.AlignVCenter
                }

                ComboBox {
                    id: platformCombo
                    Layout.preferredWidth: 148
                    Layout.preferredHeight: 36
                    model: logExtractManage.platformNames
                    enabled: root.phase !== "extracting" && !root.refreshingConn
                    opacity: enabled ? 1.0 : 0.55

                    background: Rectangle {
                        radius: 8
                        color: platformCombo.pressed || platformCombo.down ? "#0f2a1f" : "#1e293b"
                        border {
                            width: 1
                            color: (platformCombo.hovered || platformCombo.down || platformCombo.activeFocus)
                                   ? "#34d399" : "#334155"
                        }
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }
                    contentItem: Text {
                        text: platformCombo.displayText
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        color: (platformCombo.hovered || platformCombo.down || platformCombo.activeFocus)
                               ? "#34d399" : "#cbd5e1"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 12
                        rightPadding: platformCombo.indicator.width + 10
                        elide: Text.ElideRight
                    }
                    indicator: Text {
                        x: platformCombo.width - width - 12
                        y: (platformCombo.height - height) / 2
                        text: platformCombo.down ? "\u25B2" : "\u25BC"
                        font.pixelSize: 10
                        color: (platformCombo.hovered || platformCombo.down)
                               ? "#34d399" : "#94a3b8"
                    }
                    delegate: ItemDelegate {
                        width: platformCombo.width
                        height: 34
                        highlighted: platformCombo.highlightedIndex === index
                        contentItem: Text {
                            text: modelData
                            font.pixelSize: 13
                            color: highlighted ? "#34d399" : "#e2e8f0"
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 10
                        }
                        background: Rectangle {
                            color: highlighted ? "#0f2a1f" : "#0f172a"
                            radius: 4
                        }
                    }
                    popup: Popup {
                        y: platformCombo.height + 4
                        width: platformCombo.width
                        padding: 4
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: platformCombo.popup.visible ? platformCombo.delegateModel : null
                            currentIndex: platformCombo.highlightedIndex
                        }
                        background: Rectangle {
                            radius: 8
                            color: "#0f172a"
                            border { color: "#334155"; width: 1 }
                        }
                    }

                    Component.onCompleted: {
                        var ids = logExtractManage.platformIds
                        var cur = logExtractManage.activePlatformId
                        var idx = ids.indexOf(cur)
                        if (idx < 0) idx = 0
                        currentIndex = idx
                    }
                    onActivated: function(index) {
                        var ids = logExtractManage.platformIds
                        if (index < 0 || index >= ids.length)
                            return
                        logExtractManage.setActivePlatform(ids[index])
                        appendOp("[OK] 切换平台: " + logExtractManage.platformNames[index]
                                 + " (" + ids[index] + ")")
                        root.refreshConnections()
                    }
                }

                Rectangle {
                    id: refreshConnBtn
                    Layout.preferredWidth: 108
                    Layout.preferredHeight: 36
                    radius: 8
                    opacity: (root.phase === "extracting" || root.refreshingConn) ? 0.55 : 1.0
                    scale: refreshMa.pressed && refreshMa.enabled ? 0.97 : 1.0
                    color: !refreshMa.enabled ? "#1e293b"
                         : (refreshMa.pressed ? "#064e3b"
                         : (refreshMa.containsMouse ? "#0f2a1f" : "#1e293b"))
                    border {
                        width: 1
                        color: (refreshMa.enabled && (refreshMa.containsMouse || refreshMa.pressed)) ? "#34d399" : "#334155"
                    }
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on border.color { ColorAnimation { duration: 120 } }
                    Row {
                        anchors.centerIn: parent; spacing: 6
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "\u21BB"
                            font.pixelSize: 16
                            color: (refreshMa.enabled && (refreshMa.containsMouse || refreshMa.pressed)) ? "#34d399" : "#cbd5e1"
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.refreshingConn ? "刷新中" : "刷新"
                            font.pixelSize: 14; font.weight: Font.Medium
                            color: (refreshMa.enabled && (refreshMa.containsMouse || refreshMa.pressed)) ? "#34d399" : "#cbd5e1"
                        }
                    }
                    MouseArea {
                        id: refreshMa
                        anchors.fill: parent
                        hoverEnabled: root.phase !== "extracting" && !root.refreshingConn
                        enabled: root.phase !== "extracting" && !root.refreshingConn
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.refreshConnections()
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e293b" }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 16
                rowSpacing: 16

                Repeater {
                    model: [
                        { title: "YOCTO",  tip: "USB_SWITCH=Y 时直连；否则转发 127.0.0.1:7665→6665", tag: "mobilelog-Y" },
                        { title: "TBOX",   tip: "USB_SWITCH=T 时直连；否则转发 127.0.0.1:7667→6667", tag: "mobilelog-T" },
                        { title: "Android", tip: "USB_SWITCH=A 时直连；否则转发 127.0.0.1:7666→6666", tag: "mobilelog-A" }
                    ]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 210
                        radius: 10
                        color: "#0b1220"
                        border {
                            width: 1
                            color: root.systemOnlineOf(modelData.title) ? "#14532d" : "#7f1d1d"
                        }

                        Item {
                            anchors.fill: parent

                            Column {
                                anchors {
                                    top: parent.top; left: parent.left; right: parent.right
                                    topMargin: 14; leftMargin: 14; rightMargin: 14
                                }
                                spacing: 8
                                Row {
                                    spacing: 8
                                    Rectangle {
                                        width: 10; height: 10; radius: 5
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: root.systemOnlineOf(modelData.title) ? "#34d399" : "#f87171"
                                    }
                                    Text {
                                        text: modelData.title
                                        font.pixelSize: 16; font.weight: Font.Bold; color: "#e2e8f0"
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: root.systemOnlineOf(modelData.title) ? "已连接·可拉取" : "未连接·不可拉取"
                                        font.pixelSize: 12
                                        color: root.systemOnlineOf(modelData.title) ? "#34d399" : "#f87171"
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
                            }

                            Row {
                                anchors {
                                    left: parent.left; right: parent.right; bottom: parent.bottom
                                    leftMargin: 14; rightMargin: 14; bottomMargin: 15
                                }
                                spacing: 8
                                // 未连接：本系统两个按钮都不可按；三系统互不影响
                                readonly property bool sysDisabled:
                                    root.phase === "extracting" || !root.systemOnlineOf(modelData.title)
                                MiniActionBtn {
                                    width: (parent.width - 8) / 2
                                    label: "提取日志"
                                    iconSource: root.iconExtract
                                    busy: parent.sysDisabled
                                    onClicked: root.extractOne(modelData.title)
                                }
                                MiniActionBtn {
                                    width: (parent.width - 8) / 2
                                    label: "路径保存"
                                    iconSource: root.iconFolder
                                    busy: parent.sysDisabled
                                    onClicked: root.pickPathFor(modelData.title)
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
                        text: "0) 顶部「平台选择」切换平台（logcfg platforms，决定 sysenv 工具与型号表）\n"
                            + "1) adb devices                     ← 无设备则界面提示「无设备连接」\n"
                            + "2) adb -d shell <sysenv_tool> read USB_SWITCH\n"
                            + "   → Y/T/A 映射为 YOCTO / TBOX / Android（与 SWITCH 一致走直连型号）\n"
                            + "3) adb root\n"
                            + "4) 非当前 SWITCH 系统：adb -d forward tcp:<port> tcp:<port-1000>\n"
                            + "   （7665→6665 / 7666→6666 / 7667→6667）\n"
                            + "5) adb connect 127.0.0.1:<port>\n"
                            + "6) adb -s <serial> pull <remote_path> <本地目录>\n"
                            + "7) 目录名: YOCTO→mobilelog-Y  TBOX→mobilelog-T  Android→mobilelog-A\n"
                            + "   重复则 mobilelog-Y(1) / mobilelog-T(1)…（ASCII 括号）\n\n"
                            + "可对单系统点「提取日志」；「路径保存」可自定义该系统保存目录。"
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
                            text: "无设备连接 — 请检查 USB / ADB 后再进行提取日志"
                            font.pixelSize: 14; font.weight: Font.Medium; color: "#fecaca"
                        }
                    }
                }
            }
        }
    }
}
