import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    // ===== Tab 导航 =====
    Rectangle {
        id: tabBarBg
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 38; color: "#0f172a"
        radius: 6
        border { color: "#1e293b"; width: 1 }

        RowLayout {
            anchors { fill: parent; leftMargin: 2; rightMargin: 2 }
            spacing: 0

            Repeater {
                model: ["项目管理", "系统配置", "测试项"]
                Rectangle {
                    Layout.preferredWidth: 110; Layout.fillHeight: true
                    radius: 4
                    color: root.currentIndex === index ? "#1e293b" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: modelData; font.pixelSize: 13; font.weight: root.currentIndex === index ? Font.Bold : Font.Normal
                        color: root.currentIndex === index ? "#e2e8f0" : "#64748b"
                    }

                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentIndex = index
                    }
                }
            }
        }
    }

    // 当前页码（非可视，纯状态）
    property int currentIndex: 0
    onCurrentIndexChanged: {
        if (currentIndex === 0) { page1.visible = true; page2.visible = false; page3.visible = false }
        else if (currentIndex === 1) { page1.visible = false; page2.visible = true; page3.visible = false }
        else { page1.visible = false; page2.visible = false; page3.visible = true }
    }

    function readField(section, key) { return Config.readFixedField(section, key) }

    // ===== 第 1 页 =====
    Item {
        id: page1; visible: true
        anchors { left: parent.left; right: parent.right; top: tabBarBg.bottom; bottom: parent.bottom; topMargin: 8 }

        ColumnLayout {
            anchors { fill: parent; leftMargin: 4; rightMargin: 4 }
            spacing: 8

            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Text { text: "当前项目:"; font.pixelSize: 14; color: "#94a3b8"; Layout.alignment: Qt.AlignVCenter }
                ComboBox {
                    id: projCombo
                    Layout.preferredWidth: 160; Layout.preferredHeight: 34
                    model: Config.projectList()
                    currentIndex: model.indexOf(Config.currentProject())
                    onActivated: Config.loadProject(currentText)
                    background: Rectangle { radius: 6; color: "#1e293b"; border { color: "#334155"; width: 1 } }
                    contentItem: Text { text: projCombo.currentText; font.pixelSize: 13; color: "#e2e8f0"; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
                    delegate: ItemDelegate {
                        width: projCombo.width; contentItem: Text { text: modelData; font.pixelSize: 13; color: "#e2e8f0"; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: highlighted ? "#1e3a5f" : "#0f172a" }
                        highlighted: projCombo.highlightedIndex === index
                    }
                    popup: Popup {
                        y: projCombo.height + 4; width: projCombo.width; padding: 2
                        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: projCombo.popup.visible ? projCombo.delegateModel : null }
                        background: Rectangle { radius: 6; color: "#0f172a"; border { color: "#1e293b"; width: 1 } }
                    }
                }
                Btn { text: "新建"; accent: "#60a5fa"; onClicked: newDlg.open() }
                Btn { text: "删除"; accent: "#ef4444"; onClicked: {
                    var n = projCombo.currentText
                    if (projCombo.model.length > 1) { Config.deleteProject(n); projCombo.model = Config.projectList() }
                }}
                Btn { text: "保存"; accent: "#22c55e"; onClicked: Config.saveCurrentProject() }
                Item { Layout.fillWidth: true }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#1e293b" }

            Flickable {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                contentHeight: fixedCol.implicitHeight + 8
                ColumnLayout {
                    id: fixedCol; anchors { left: parent.left; right: parent.right }
                    spacing: 6
                    SectionTitle { text: "工厂信息" }
                    GridLayout { columns: 4; columnSpacing: 16; rowSpacing: 6; Layout.fillWidth: true
                        Fld { label: "工厂名称"; section: "factory"; key: "factory_name" }
                        Fld { label: "MES IP";  section: "factory"; key: "mes_ip" }
                        Fld { label: "MES 端口"; section: "factory"; key: "mes_port" }
                        Fld { label: "MES 启用"; section: "factory"; key: "mes_enable" }
                    }
                    SectionTitle { text: "工位信息" }
                    GridLayout { columns: 4; columnSpacing: 16; rowSpacing: 6; Layout.fillWidth: true
                        Fld { label: "员工工号"; section: "station"; key: "user_no" }
                        Fld { label: "工单号";  section: "station"; key: "order_no" }
                        Fld { label: "线别";    section: "station"; key: "line_name" }
                        Fld { label: "工序名称"; section: "station"; key: "station_name" }
                        Fld { label: "夹具编号"; section: "station"; key: "fixture_no" }
                    }
                    SectionTitle { text: "连接串口" }
                    GridLayout { columns: 4; columnSpacing: 16; rowSpacing: 6; Layout.fillWidth: true
                        Fld { label: "COM";    section: "connect_serial"; key: "com" }
                        Fld { label: "波特率"; section: "connect_serial"; key: "baud_rate" }
                    }
                    SectionTitle { text: "调试串口" }
                    GridLayout { columns: 4; columnSpacing: 16; rowSpacing: 6; Layout.fillWidth: true
                        Fld { label: "COM";    section: "debug_serial"; key: "com" }
                        Fld { label: "波特率"; section: "debug_serial"; key: "baud_rate" }
                    }
                }
            }
        }

        Popup {
            id: newDlg; width: 280; height: 140; x: (root.width - width) / 2; y: 80
            padding: 16; closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
            background: Rectangle { radius: 8; color: "#0f172a"; border { color: "#1e293b"; width: 1 } }
            contentItem: ColumnLayout { spacing: 12
                Text { text: "新建项目"; font.pixelSize: 16; font.weight: Font.Bold; color: "#e2e8f0" }
                RowLayout { spacing: 8
                    TextField {
                        id: newName; Layout.fillWidth: true; font.pixelSize: 13; color: "#e2e8f0"
                        placeholderText: "项目名称(不含.json)"; placeholderTextColor: "#475569"
                        background: Rectangle { radius: 4; color: "#1e293b"; border { color: "#334155"; width: 1 } }
                    }
                    Btn { text: "创建"; accent: "#22c55e"; onClicked: {
                        if (newName.text) { Config.createProject(newName.text); projCombo.model = Config.projectList(); Config.loadProject(newName.text); projCombo.currentIndex = projCombo.model.indexOf(newName.text); newDlg.close() }
                    }}
                }
            }
        }
    }

    // ===== 第 2 页：环境变量 =====
    Item {
        id: page2; visible: false
        anchors { left: parent.left; right: parent.right; top: tabBarBg.bottom; bottom: parent.bottom; topMargin: 8 }

        ColumnLayout {
            anchors.fill: parent; spacing: 4
            RowLayout {
                Layout.fillWidth: true; height: 38
                Item { Layout.fillWidth: true }
                Btn { text: "+ 添加"; accent: "#22c55e"; onClicked: { Config.addEnvItem(m4("name","","descr","","value","","type","string")); envModel = Config.envItemsForQml() } }
            }
            // 表头
            RowLayout { Layout.fillWidth: true; height: 30; spacing: 4
                Hdr2 { Layout.preferredWidth: p2w(0.15); text: "name" }
                Hdr2 { Layout.preferredWidth: p2w(0.30); text: "descr" }
                Hdr2 { Layout.preferredWidth: p2w(0.40); text: "value" }
                Hdr2 { Layout.preferredWidth: p2w(0.15); text: "type" }
                Item { width: 56 }
            }
            ListView {
                id: envList2
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 2
                model: Config.envItemsForQml()
                property var envModel: model
                delegate: RowLayout { height: 44; spacing: 4
                    Edt2 { Layout.preferredWidth: p2w(0.15); text: modelData.name;  onEditingFinished: Config.updateEnvItem(index, m1("name", text)) }
                    Edt2 { Layout.preferredWidth: p2w(0.30); text: modelData.descr; onEditingFinished: Config.updateEnvItem(index, m1("descr", text)) }
                    Edt2 { Layout.preferredWidth: p2w(0.40); text: modelData.value; onEditingFinished: Config.updateEnvItem(index, m1("value", text)) }
                    Edt2 { Layout.preferredWidth: p2w(0.15); text: modelData.type;  onEditingFinished: Config.updateEnvItem(index, m1("type", text)) }
                    DelBtn2 { onClicked: { Config.removeEnvItem(index); envList2.model = Config.envItemsForQml() } }
                }
            }
        }
    }

    // ===== 第 3 页：测试项 =====
    Item {
        id: page3; visible: false
        anchors { left: parent.left; right: parent.right; top: tabBarBg.bottom; bottom: parent.bottom; topMargin: 8 }

        ColumnLayout {
            anchors.fill: parent; spacing: 4
            RowLayout {
                Layout.fillWidth: true; height: 38
                Item { Layout.fillWidth: true }
                Btn { text: "+ 添加"; accent: "#22c55e"; onClicked: { Config.addTestItem(m4("code",0,"value",true,"descr","新测试项","timeout",120)); testList3.model = Config.testItemsForQml() } }
            }
            RowLayout { Layout.fillWidth: true; height: 30; spacing: 4
                Hdr2 { Layout.preferredWidth: 40; text: "启" }
                Hdr2 { Layout.preferredWidth: p3w(0.12); text: "code" }
                Hdr2 { Layout.preferredWidth: p3w(0.52); text: "descr" }
                Hdr2 { Layout.preferredWidth: p3w(0.18); text: "超时(s)" }
                Hdr2 { Layout.preferredWidth: p3w(0.18); text: "重试" }
                Item { width: 56 }
            }
            ListView {
                id: testList3
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 2
                model: Config.testItemsForQml()
                delegate: RowLayout { height: 44; spacing: 4
                    CheckBox {
                        Layout.preferredWidth: 40; checked: modelData.value
                        onToggled: Config.updateTestItem(index, m1("value", checked))
                        indicator: Rectangle { implicitWidth: 16; implicitHeight: 16; radius: 3; color: parent.checked ? "#166534" : "#1e293b"; border { color: parent.checked ? "#22c55e" : "#334155"; width: 1 } }
                    }
                    Edt2 { Layout.preferredWidth: p3w(0.12); text: modelData.code;    onEditingFinished: Config.updateTestItem(index, m1("code", parseInt(text)||0)) }
                    Edt2 { Layout.preferredWidth: p3w(0.52); text: modelData.descr;   onEditingFinished: Config.updateTestItem(index, m1("descr", text)) }
                    Edt2 { Layout.preferredWidth: p3w(0.18); text: modelData.timeout;  onEditingFinished: Config.updateTestItem(index, m1("timeout", parseInt(text)||0)) }
                    Edt2 { Layout.preferredWidth: p3w(0.18); text: modelData.retries;  onEditingFinished: Config.updateTestItem(index, m1("retries", parseInt(text)||0)) }
                    DelBtn2 { onClicked: { Config.removeTestItem(index); testList3.model = Config.testItemsForQml() } }
                }
            }
        }
    }

    // 动态宽度计算：可用宽 = 父容器宽 - 56(删除按钮) - 16(间距)
    function p2w(ratio) { return (width - 72) * ratio }
    function p3w(ratio) { return (width - 56 - 40 - 16) * ratio }

    // ===== JS =====
    function m1(k, v) { var m = {}; m[k] = v; return m }
    function m4(k1, v1, k2, v2, k3, v3, k4, v4) {
        var m = {}; m[k1] = v1; m[k2] = v2; m[k3] = v3; m[k4] = v4; return m
    }

    // ===== 共用组件 =====
    component SectionTitle : Text {
        Layout.fillWidth: true; Layout.topMargin: 10
        font.pixelSize: 15; font.weight: Font.Bold; color: "#94a3b8"
    }

    component Fld : RowLayout {
        property string label; property string section; property string key
        signal editingFinished(string text)
        spacing: 6
        Text { text: label; font.pixelSize: 14; color: "#64748b"; Layout.preferredWidth: 80; Layout.alignment: Qt.AlignVCenter }
        TextField {
            id: fi; Layout.preferredWidth: 200; font.pixelSize: 14; color: "#e2e8f0"
            text: Config.readFixedField(section, key)
            verticalAlignment: Text.AlignVCenter; leftPadding: 6; selectByMouse: true
            background: Rectangle { radius: 3; color: "#1e293b"; border { color: "#334155"; width: 1 } }
            onEditingFinished: parent.editingFinished(text)
        }
    }

    component Btn : Rectangle {
        property string text: ""; property color accent: "#60a5fa"
        width: 64; height: 30; radius: 4; color: ma.containsMouse ? Qt.lighter(accent, 1.2) : "transparent"
        border { color: ma.containsMouse ? accent : "#334155"; width: 1 }
        signal clicked()
        Text { anchors.centerIn: parent; text: parent.text; font.pixelSize: 12; color: ma.containsMouse ? "#fff" : parent.accent }
        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
    }

    component Hdr2 : Text { font.pixelSize: 16; color: "#64748b"; font.weight: Font.Bold }

    component Edt2 : TextField {
        font.pixelSize: 14; color: "#e2e8f0"; verticalAlignment: Text.AlignVCenter
        leftPadding: 8; rightPadding: 8; selectByMouse: true
        background: Rectangle { radius: 3; color: "#1e293b"; border { color: "#334155"; width: 1 } }
    }

    component DelBtn2 : Rectangle {
        width: 48; height: 32; radius: 4; color: dm.containsMouse ? "#7f1d1d" : "transparent"
        border { color: dm.containsMouse ? "#ef4444" : "#334155"; width: 1 }
        signal clicked()
        Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 14; color: dm.containsMouse ? "#f87171" : "#64748b" }
        MouseArea { id: dm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: parent.clicked() }
    }
}
