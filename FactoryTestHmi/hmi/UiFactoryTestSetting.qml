import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FactoryTestHmi 1.0   // 确保 Config 在此 URI 下

Rectangle {
    id: root
    color: "#020617"
    anchors.fill: parent
    property var currentProject: Config.currentProject()

    // 滚动区域
    ScrollView {
        anchors.fill: parent
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: root.width - 32
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 16

            // ========== 标题栏 ==========
            RowLayout {
                Layout.topMargin: 24
                Layout.fillWidth: true
                spacing: 12
                Canvas {
                    width: 24; height: 24
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.strokeStyle = "#60a5fa"
                        ctx.lineWidth = 2
                        ctx.beginPath()
                        ctx.arc(12,12,10,0,2*Math.PI)
                        ctx.stroke()
                        ctx.beginPath()
                        ctx.moveTo(12,4); ctx.lineTo(12,20)
                        ctx.moveTo(4,12); ctx.lineTo(20,12)
                        ctx.stroke()
                    }
                }
                Text {
                    text: "测试参数配置"
                    font.pixelSize: 22; font.weight: Font.Bold
                    color: "#e2e8f0"
                }
            }

            // ========== 项目选择 ==========
            Rectangle {
                Layout.fillWidth: true
                height: 60; radius: 10
                color: "#0f172a"; border { color: "#1e293b"; width: 1 }
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    Text { text: "当前项目:"; font.pixelSize: 16; color: "#94a3b8" }
                    ComboBox {
                        id: projectCombo
                        model: ["P39", "P41", "P4A"]
                        currentIndex: model.indexOf(currentProject)
                        onActivated: {
                            Config.loadProject(currentText)
                            // 刷新所有数据，通过重新加载组件或触发模型更新
                            factoryBox.data = Config.factoryConfigMap()
                            stationBox.data = Config.stationConfigMap()
                            connectBox.data = Config.connectSerialMap()
                            debugBox.data = Config.debugSerialMap()
                            envRepeater.model = Config.envItemsList()
                            testRepeater.model = Config.allTestItemsList()
                        }
                    }
                }
            }

            // ========== 工厂配置卡片 ==========
            ConfigCard {
                id: factoryBox
                title: "工厂配置"
                iconColor: "#f97316"
                data: Config.factoryConfigMap()
                fields: [
                    { label: "工厂名称", key: "factory_name", editable: false },
                    { label: "MES IP", key: "mes_ip", editable: true },
                    { label: "MES 端口", key: "mes_port", editable: true },
                    { label: "上传 MES", key: "mes_enable", editable: true, type: "switch" }
                ]
            }

            // ========== 站位配置卡片 ==========
            ConfigCard {
                id: stationBox
                title: "站位配置"
                iconColor: "#3b82f6"
                data: Config.stationConfigMap()
                fields: [
                    { label: "员工编号", key: "user_no", editable: true },
                    { label: "工单号", key: "order_no", editable: true },
                    { label: "线别", key: "line_name", editable: true },
                    { label: "工序名称", key: "station_name", editable: true },
                    { label: "夹具编号", key: "fixture_no", editable: true }
                ]
            }

            // ========== 串口配置卡片 ==========
            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                ConfigCard {
                    id: connectBox
                    Layout.fillWidth: true
                    title: "通信串口"
                    iconColor: "#60a5fa"
                    data: Config.connectSerialMap()
                    fields: [
                        { label: "COM", key: "com", editable: true, type: "combobox", options: ["COM1","COM2","COM3","COM4","COM5"] },
                        { label: "波特率", key: "baud_rate", editable: true, type: "combobox", options: ["9600","115200","921600"] }
                    ]
                }
                ConfigCard {
                    id: debugBox
                    Layout.fillWidth: true
                    title: "调试串口"
                    iconColor: "#8b5cf6"
                    data: Config.debugSerialMap()
                    fields: [
                        { label: "COM", key: "com", editable: true, type: "combobox", options: ["COM1","COM2","COM3","COM4"] },
                        { label: "波特率", key: "baud_rate", editable: true, type: "combobox", options: ["9600","115200"] }
                    ]
                }
            }

            // ========== 环境变量卡片 ==========
            Rectangle {
                Layout.fillWidth: true
                radius: 10
                color: "#0f172a"; border { color: "#1e293b"; width: 1 }
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16
                    RowLayout {
                        Canvas {
                            width: 18; height: 18
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.fillStyle = "#10b981"; ctx.beginPath(); ctx.arc(9,9,7,0,2*Math.PI); ctx.fill()
                            }
                        }
                        Text { text: "环境变量"; font.pixelSize: 16; color: "#e2e8f0" }
                    }
                    ListView {
                        Layout.fillWidth: true; Layout.preferredHeight: contentHeight
                        interactive: false
                        model: Config.envItemsList()
                        delegate: RowLayout {
                            width: ListView.view.width
                            spacing: 8
                            Text { text: modelData.descr; Layout.preferredWidth: 120; font.pixelSize: 14; color: "#cbd5e1" }
                            TextField {
                                Layout.fillWidth: true
                                text: modelData.value; font.pixelSize: 14; color: "#e2e8f0"
                                background: Rectangle { radius: 4; color: "#1e293b"; border.color: "#334155" }
                            }
                        }
                    }
                }
            }

            // ========== 测试项目卡片 ==========
            Rectangle {
                Layout.fillWidth: true
                radius: 10
                color: "#0f172a"; border { color: "#1e293b"; width: 1 }
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 16
                    RowLayout {
                        Canvas {
                            width: 18; height: 18
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.fillStyle = "#a78bfa"; ctx.beginPath(); ctx.arc(9,9,7,0,2*Math.PI); ctx.fill()
                            }
                        }
                        Text { text: "测试项列表"; font.pixelSize: 16; color: "#e2e8f0" }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "保存测试项"
                            onClicked: {
                                var list = [];
                                for (var i=0; i<testRepeater.count; i++) {
                                    var item = testRepeater.itemAt(i);
                                    list.push(item.getData());
                                }
                                Config.saveTestItemsFromList(list);
                            }
                            palette.button: "#166534"
                            palette.buttonText: "#e2e8f0"
                        }
                    }
                    ListView {
                        id: testListView
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(contentHeight, 500)
                        clip: true
                        model: Config.allTestItemsList()
                        delegate: testItemDelegate
                    }
                }
            }

            Component {
                id: testItemDelegate
                Item {
                    id: testDelegate
                    width: testListView.width
                    height: 48
                    property var modelMap: modelData
                    function getData() {
                        return {
                            code: switchEnabled.code,
                            sn: modelMap.sn,
                            value: switchEnabled.checked,
                            descr: modelMap.descr,
                            timeout: parseInt(timeoutInput.text),
                            retries: parseInt(retriesInput.text)
                        };
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 8
                        Switch {
                            id: switchEnabled
                            checked: modelMap.value
                            property int code: modelMap.code
                        }
                        Text {
                            text: modelMap.descr
                            Layout.preferredWidth: 160
                            font.pixelSize: 14; color: "#e2e8f0"
                        }
                        Text { text: "超时(s)"; font.pixelSize: 12; color: "#94a3b8" }
                        TextField {
                            id: timeoutInput
                            text: modelMap.timeout
                            Layout.preferredWidth: 50
                            font.pixelSize: 14; color: "#e2e8f0"
                            background: Rectangle { radius: 4; color: "#1e293b"; border.color: "#334155" }
                        }
                        Text { text: "重试"; font.pixelSize: 12; color: "#94a3b8" }
                        TextField {
                            id: retriesInput
                            text: modelMap.retries
                            Layout.preferredWidth: 50
                            font.pixelSize: 14; color: "#e2e8f0"
                            background: Rectangle { radius: 4; color: "#1e293b"; border.color: "#334155" }
                        }
                    }
                }
            }
        }
    }

    // 通用卡片组件
    component ConfigCard : Rectangle {
        id: card
        radius: 10
        color: "#0f172a"; border { color: "#1e293b"; width: 1 }
        property string title
        property color iconColor
        property var data
        property var fields: []

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16
            spacing: 8
            RowLayout {
                Canvas {
                    width: 18; height: 18
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.fillStyle = iconColor
                        ctx.beginPath(); ctx.arc(9,9,7,0,2*Math.PI); ctx.fill()
                    }
                }
                Text { text: title; font.pixelSize: 16; color: "#e2e8f0" }
            }
            Repeater {
                model: fields
                delegate: RowLayout {
                    width: card.width - 32
                    spacing: 8
                    Text { text: modelData.label; Layout.preferredWidth: 100; font.pixelSize: 14; color: "#cbd5e1" }
                    Text {
                        visible: modelData.type !== "switch" && modelData.type !== "combobox"
                        Layout.fillWidth: true
                        text: data[modelData.key] !== undefined ? data[modelData.key] : ""
                        font.pixelSize: 14; color: "#e2e8f0"
                    }
                    ComboBox {
                        visible: modelData.type === "combobox"
                        Layout.fillWidth: true
                        model: modelData.options
                        currentIndex: modelData.options.indexOf(data[modelData.key])
                    }
                    Switch {
                        visible: modelData.type === "switch"
                        checked: data[modelData.key] === true
                    }
                }
            }
        }
    }
}