import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: testPanel
    color: "transparent"

    // 测试数据（与原始 HTML 一致）
    property var testItems: [
        { id: 1,  name: "外观结构完整性检查",       std: "视觉无损",           value: "确认通过",  status: "finished",  result: "pass"    },
        { id: 2,  name: "输入端绝缘电阻测试",       std: "> 500MΩ",            value: "852MΩ",   status: "finished",  result: "pass"    },
        { id: 3,  name: "主电路通电自检",           std: "握手成功",           value: "OK",      status: "finished",  result: "pass"    },
        { id: 4,  name: "静态待机电流校验",         std: "< 50mA",             value: "32.5mA",  status: "finished",  result: "pass"    },
        { id: 5,  name: "输出电压偏差测量 (12V)",    std: "11.8~12.2V",        value: "11.95V",  status: "processing",result: "pending" },
        { id: 6,  name: "满载纹波噪声测试",         std: "< 100mVp-p",         value: "--",      status: "waiting",   result: "pending" },
        { id: 7,  name: "过流保护动作点验证",       std: "120%~150%",          value: "--",      status: "waiting",   result: "pending" },
        { id: 8,  name: "过压保护响应时间",         std: "< 50ms",             value: "--",      status: "waiting",   result: "pending" },
        { id: 9,  name: "CAN总线通讯稳定性",        std: "丢包率=0",           value: "--",      status: "waiting",   result: "pending" },
        { id: 10, name: "温升模拟负载测试",         std: "< 65°C",             value: "--",      status: "waiting",   result: "pending" },
        { id: 11, name: "外壳接地导通电阻",         std: "< 0.1Ω",             value: "--",      status: "waiting",   result: "pending" },
        { id: 12, name: "动态负载切换响应",         std: "< 2ms",              value: "--",      status: "waiting",   result: "pending" },
        { id: 13, name: "EMC 电磁兼容初筛",         std: "符合 A 级",          value: "--",      status: "waiting",   result: "pending" },
        { id: 14, name: "产品序列号唯一性核对",     std: "有效SN",             value: "--",      status: "waiting",   result: "pending" },
        { id: 15, name: "测试固件版本核实",         std: "V1.5.2+",            value: "--",      status: "waiting",   result: "pending" }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // ---------- 1. 实时图表区 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 280
            radius: 12
            color: "transparent"
            border.color: "#1e293b"
            border.width: 1
            // 背景渐变
            Rectangle {
                anchors.fill: parent
                radius: 12
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#0f172a" }
                    GradientStop { position: 1.0; color: "#020617" }
                }
                border.color: "#10ffffff"
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                // 图表标题行
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "📈 近1小时合格率波动趋势"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: "#94a3b8"
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: "更新频率: 1s"
                        font.pixelSize: 12
                        color: "#64748b"
                    }
                }

                // 折线图画布
                Canvas {
                    id: trendCanvas
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // X轴标签
                    property var labels: ["10:00","10:05","10:10","10:15","10:20","10:25","10:30","10:35","10:40","10:45"]
                    // 数据点
                    property var data: [98.2, 98.5, 97.8, 99.1, 98.6, 98.4, 99.0, 98.8, 97.5, 98.4]
                    property double yMin: 90
                    property double yMax: 100

                    onPaint: {
                        var ctx = getContext("2d");
                        var w = width;
                        var h = height;

                        ctx.clearRect(0, 0, w, h);

                        // 网格线 & Y轴标注
                        var leftPad = 38;
                        var rightPad = 16;
                        var topPad = 12;
                        var bottomPad = 28;
                        var plotW = w - leftPad - rightPad;
                        var plotH = h - topPad - bottomPad;

                        ctx.strokeStyle = "#1e293b";
                        ctx.lineWidth = 1;
                        // 水平网格线 (5条)
                        var yTicks = [90, 92.5, 95, 97.5, 100];
                        for (var i = 0; i < yTicks.length; i++) {
                            var y = topPad + plotH * (1 - (yTicks[i] - yMin) / (yMax - yMin));
                            ctx.beginPath();
                            ctx.moveTo(leftPad, y);
                            ctx.lineTo(w - rightPad, y);
                            ctx.stroke();

                            ctx.fillStyle = "#94a3b8";
                            ctx.font = "11px 'Inter', sans-serif";
                            ctx.textAlign = "right";
                            ctx.fillText(yTicks[i] + "%", leftPad - 6, y + 4);
                        }

                        // X轴标签
                        ctx.fillStyle = "#94a3b8";
                        ctx.textAlign = "center";
                        for (var j = 0; j < labels.length; j++) {
                            var x = leftPad + (j / (labels.length - 1)) * plotW;
                            ctx.fillText(labels[j], x, h - 4);
                        }

                        // 绘制折线
                        ctx.strokeStyle = "#3b82f6";
                        ctx.lineWidth = 3;
                        ctx.beginPath();
                        for (var k = 0; k < data.length; k++) {
                            var px = leftPad + (k / (data.length - 1)) * plotW;
                            var py = topPad + plotH * (1 - (data[k] - yMin) / (yMax - yMin));
                            if (k === 0) ctx.moveTo(px, py);
                            else ctx.lineTo(px, py);
                        }
                        ctx.stroke();

                        // 渐变填充区域
                        ctx.save();
                        ctx.globalAlpha = 0.15;
                        var gradient = ctx.createLinearGradient(0, topPad, 0, topPad + plotH);
                        gradient.addColorStop(0, "#3b82f6");
                        gradient.addColorStop(1, "transparent");
                        ctx.fillStyle = gradient;
                        ctx.beginPath();
                        ctx.moveTo(leftPad, topPad + plotH);
                        for (var l = 0; l < data.length; l++) {
                            var px2 = leftPad + (l / (data.length - 1)) * plotW;
                            var py2 = topPad + plotH * (1 - (data[l] - yMin) / (yMax - yMin));
                            ctx.lineTo(px2, py2);
                        }
                        ctx.lineTo(leftPad + plotW, topPad + plotH);
                        ctx.closePath();
                        ctx.fill();
                        ctx.restore();

                        // 数据点圆点
                        ctx.fillStyle = "#3b82f6";
                        for (var m = 0; m < data.length; m++) {
                            var px3 = leftPad + (m / (data.length - 1)) * plotW;
                            var py3 = topPad + plotH * (1 - (data[m] - yMin) / (yMax - yMin));
                            ctx.beginPath();
                            ctx.arc(px3, py3, 4, 0, Math.PI * 2);
                            ctx.fill();
                        }
                    }
                }
            }
        }

        // ---------- 2. 测试项目表格 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true   // 占据剩余空间
            radius: 12
            color: "transparent"
            border.color: "#1e293b"
            border.width: 1
            clip: true
            Rectangle {
                anchors.fill: parent
                radius: 12
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#0f172a" }
                    GradientStop { position: 1.0; color: "#020617" }
                }
                border.color: "#10ffffff"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 表头
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    color: "#0a0f1c"
                    border.color: "#1e293b"
                    border.width: 1

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        // 列宽度比例
                        property double col1: 0.09
                        property double col2: 0.32
                        property double col3: 0.17
                        property double col4: 0.17
                        property double col5: 0.16
                        property double col6: 0.09

                        Item { width: parent.width * col1; height: parent.height
                            Text {
                                anchors.centerIn: parent
                                text: "序号"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: "#64748b"
                            }
                        }
                        Item { width: parent.width * col2; height: parent.height
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                x: 16
                                text: "测试项目名称"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: "#64748b"
                            }
                        }
                        Item { width: parent.width * col3; height: parent.height
                            Text {
                                anchors.centerIn: parent
                                text: "标准值范围"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: "#64748b"
                            }
                        }
                        Item { width: parent.width * col4; height: parent.height
                            Text {
                                anchors.centerIn: parent
                                text: "实时测量值"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: "#64748b"
                            }
                        }
                        Item { width: parent.width * col5; height: parent.height
                            Text {
                                anchors.centerIn: parent
                                text: "当前状态"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: "#64748b"
                            }
                        }
                        Item { width: parent.width * col6; height: parent.height
                            Text {
                                anchors.centerIn: parent
                                text: "结果"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                                color: "#64748b"
                            }
                        }
                    }
                }

                // 列表内容（可滚动）
                ListView {
                    id: testListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: testItems
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                        width: 6
                        contentItem: Rectangle { color: "#334155"; radius: 3 }
                    }
                    delegate: Rectangle {
                        width: testListView.width
                        height: 48
                        color: index % 2 === 0 ? "#0a0f1c" : "#0d1321"
                        border.color: "#1e293b"
                        border.width: 1

                        // 鼠标悬停效果
                        Rectangle {
                            anchors.fill: parent
                            color: "#2a3441"
                            opacity: mouseArea.containsMouse ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 150 } }
                        }
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            // 列比例同表头
                            property double col1: 0.09
                            property double col2: 0.32
                            property double col3: 0.17
                            property double col4: 0.17
                            property double col5: 0.16
                            property double col6: 0.09

                            // 序号
                            Item { width: parent.width * col1; height: parent.height
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.id.toString().padStart(2, '0')
                                    font.pixelSize: 14
                                    color: "#64748b"
                                }
                            }
                            // 项目名
                            Item { width: parent.width * col2; height: parent.height
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: 16
                                    text: modelData.name
                                    font.pixelSize: 14
                                    color: "#e2e8f0"
                                }
                            }
                            // 标准值
                            Item { width: parent.width * col3; height: parent.height
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.std
                                    font.pixelSize: 13
                                    color: "#94a3b8"
                                }
                            }
                            // 实时值
                            Item { width: parent.width * col4; height: parent.height
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.value
                                    font.pixelSize: 13
                                    color: "#93c5fd"
                                }
                            }
                            // 状态（含图标）
                            Item { width: parent.width * col5; height: parent.height
                                Row {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    // 图标
                                    Text {
                                        text: modelData.status === "finished" ? "✔" :
                                                modelData.status === "processing" ? "🔄" : "⏳"
                                        font.pixelSize: 14
                                        color: modelData.status === "finished" ? "#22c55e" :
                                                modelData.status === "processing" ? "#60a5fa" : "#475569"
                                    }
                                    Text {
                                        text: modelData.status === "finished" ? "已完成" :
                                                modelData.status === "processing" ? "进行中" : "等待中"
                                        font.pixelSize: 13
                                        color: modelData.status === "finished" ? "#22c55e" :
                                                modelData.status === "processing" ? "#60a5fa" : "#475569"
                                    }
                                }
                            }
                            // 结果
                            Item { width: parent.width * col6; height: parent.height
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.result === "pass" ? "PASS" :
                                            modelData.result === "fail" ? "FAIL" : "--"
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                    color: modelData.result === "pass" ? "#22c55e" :
                                            modelData.result === "fail" ? "#ef4444" : "#475569"
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---------- 3. 底部日志栏 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: 12
            color: "#020617"
            border.color: "#1e293b"
            border.width: 1

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                spacing: 12
                Text {
                    text: "[10:45:22]"
                    font.pixelSize: 12
                    color: "#3b82f6"
                    font.family: "'Courier New', monospace"
                }
                Text {
                    text: "系统就绪，等待扫描产品序列号..."
                    font.pixelSize: 12
                    color: "#94a3b8"
                }
            }
        }
    }

    // 图表画布尺寸变化时重绘
    onWidthChanged: trendCanvas.requestPaint()
    onHeightChanged: trendCanvas.requestPaint()
}