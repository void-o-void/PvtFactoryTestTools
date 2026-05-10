import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: controlCard       // 更安全的 id，避免与 root 保留属性冲突

    // ---------- 公开状态 ----------
    property bool isRunning: false
    property int elapsedMs: 0

    function formatTime(ms) {
        var h = Math.floor(ms / 3600000)
        var m = Math.floor((ms % 3600000) / 60000)
        var s = Math.floor((ms % 60000) / 1000)
        var z = ms % 1000
        return String(h).padStart(2, '0') + ":" +
            String(m).padStart(2, '0') + ":" +
            String(s).padStart(2, '0') + "." +
            String(z).padStart(3, '0')
    }

    function toggleTest() {
        if (!isRunning) {
            elapsedMs = 0
            isRunning = true
        } else {
            isRunning = false
        }
    }

    // 10ms 计时器
    Timer {
        id: runTimer
        interval: 10
        running: controlCard.isRunning
        repeat: true
        onTriggered: elapsedMs += 10
    }

    // 数字字体（可选）
    FontLoader {
        id: digitalFont
        source: "https://fonts.cdnfonts.com/s/14352/DigitalNumbers-Regular.woff"
        onStatusChanged: {
            if (status === FontLoader.Error)
                console.warn("数字字体加载失败，使用等宽字体代替")
        }
    }

    implicitWidth: 480
    implicitHeight: 580
    color: "transparent"

    // 工业风渐变背景
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
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: 12
            bottom: parent.bottom
            bottomMargin: 24
            leftMargin: 24
            rightMargin: 24
        }
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Canvas {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                Layout.alignment: Qt.AlignBottom
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.fillStyle = "#60a5fa"
                    ctx.fillRect(1, 1, 7, 7); ctx.fillRect(3, 3, 3, 3)
                    ctx.fillStyle = "#0f172a"; ctx.fillRect(3, 3, 3, 3)
                    ctx.fillStyle = "#60a5fa"; ctx.fillRect(11, 1, 6, 7); ctx.fillRect(13, 3, 3, 3)
                    ctx.fillStyle = "#0f172a"; ctx.fillRect(13, 3, 3, 3)
                    ctx.fillStyle = "#60a5fa"; ctx.fillRect(1, 11, 7, 6); ctx.fillRect(3, 13, 3, 3)
                    ctx.fillStyle = "#0f172a"; ctx.fillRect(3, 13, 3, 3)
                    ctx.fillStyle = "#60a5fa"
                    ctx.fillRect(12, 12, 2, 2); ctx.fillRect(15, 14, 2, 2); ctx.fillRect(13, 16, 2, 2)
                }
                Component.onCompleted: requestPaint()
            }

            // 提示标签
            Label {
                text: "输入SN:"
                font.pixelSize: 18
                color: "#94a3b8"
                Layout.alignment: Qt.AlignBottom
            }

            // 输入框：只保留底部边框，聚焦高亮
            TextField {
                id: snInput
                Layout.fillWidth: true
                font.pixelSize: 22
                color: "#e2e8f0"
                background: Item { }              // 去掉所有边框
                bottomPadding: 4                  // 文字离底线一点距离

                // 底部高亮线（颜色自动随焦点切换 ——— 现成焦点机制）
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: snInput.activeFocus ? "#1976D2" : "#64748b"
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                // 按回车让输入框失焦（底部线恢复）—— 就这一行
                Keys.onReturnPressed: focus = false
            }
        }

        // ---------- 圆形启动/暂停按钮 ----------
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 256
            Layout.preferredHeight: 256

            focusPolicy: Qt.NoFocus          // 彻底禁止焦点

            Canvas {
                id: buttonCanvas
                anchors.fill: parent
                property bool running: controlCard.isRunning
                onRunningChanged: requestPaint()
                Component.onCompleted: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    var cx = width / 2, cy = height / 2, r = width / 2

                    ctx.clearRect(0, 0, width, height)

                    // 外环（深色边框）
                    ctx.beginPath()
                    ctx.arc(cx, cy, r, 0, 2 * Math.PI)
                    ctx.fillStyle = "#1e293b"
                    ctx.fill()

                    // 内部渐变圆（半径减8）
                    var grad = ctx.createRadialGradient(cx, cy, r * 0.1, cx, cy, r - 8)
                    if (controlCard.isRunning) {
                        grad.addColorStop(0, "#22c55e")
                        grad.addColorStop(1, "#166534")
                    } else {
                        grad.addColorStop(0, "#1e293b")
                        grad.addColorStop(1, "#0f172a")
                    }
                    ctx.beginPath()
                    ctx.arc(cx, cy, r - 8, 0, 2 * Math.PI)
                    ctx.fillStyle = grad
                    ctx.fill()
                }
            }

            // 中心图标 + 文字
            Column {
                anchors.centerIn: parent
                spacing: 6

                // 用纯图形替代 emoji Text，避免字体渲染产生的蓝色背景
                Item {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 55
                    height: 65

                    // 暂停图标：两条竖线
                    Row {
                        anchors.centerIn: parent
                        visible: controlCard.isRunning
                        spacing: 14
                        Rectangle { width: 22; height: 70; radius: 3; color: "white" }
                        Rectangle { width: 22; height: 70; radius: 3; color: "white" }
                    }

                    // 播放图标：三角形
                    Canvas {
                        anchors.centerIn: parent
                        visible: !controlCard.isRunning
                        width: 55
                        height: 65
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.fillStyle = "#94a3b8"
                            ctx.beginPath()
                            ctx.moveTo(8, 0)
                            ctx.lineTo(width, height / 2)
                            ctx.lineTo(8, height)
                            ctx.closePath()
                            ctx.fill()
                        }
                        Component.onCompleted: requestPaint()
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: controlCard.isRunning ? "暂停测试" : "开始测试"
                    font.pixelSize: 18
                    font.bold: true
                    font.capitalization: Font.AllUppercase
                    color: controlCard.isRunning ? "white" : "#94a3b8"
                }
            }

            // 点击区域
            MouseArea {
                anchors.fill: parent
                activeFocusOnTab: false       // 防止 Tab 键获取焦点
                acceptedButtons: Qt.LeftButton
                onClicked: controlCard.toggleTest()
            }

            // 高亮外环（半透明）
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "transparent"
                border {
                    width: 8
                    color: controlCard.isRunning ? "#22c55e" : "#1e293b"
                }
                opacity: controlCard.isRunning ? 0.4 : 1.0
            }
        }

        // ---------- 计时器 ----------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            radius: 8
            color: "#0f172a"
            border { color: "#1e293b"; width: 1 }
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 4
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "本次测试用时"
                    font.pixelSize: 11
                    font.letterSpacing: 1
                    color: "#64748b"
                }
                Text {
                    id: timerDisplay
                    Layout.alignment: Qt.AlignHCenter
                    text: formatTime(elapsedMs)
                    font.family: digitalFont.status === FontLoader.Ready ? digitalFont.name : "Courier New"
                    font.pixelSize: 40
                    font.bold: true
                    color: "#60a5fa"
                }
            }
        }

        // ---------- 操作按钮 ----------
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // 测试设置
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 8
                color: testSettingMouse.containsMouse ? "#0f2a1f" : "#1e293b"
                border {
                    color: testSettingMouse.containsMouse ? "#34d399" : "#334155"
                    width: 1
                }
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        text: "⚙"; font.pixelSize: 28
                        color: testSettingMouse.containsMouse ? "#34d399" : "#cbd5e1"
                    }
                    Text {
                        text: "测试设置"; font.pixelSize: 18
                        color: testSettingMouse.containsMouse ? "#34d399" : "#cbd5e1"
                    }
                }
                MouseArea {
                    id: testSettingMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("测试设置")
                }
            }

            // 导出结果
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                radius: 8
                color: exportMouse.containsMouse ? "#0f2a1f" : "#1e293b"
                border {
                    color: exportMouse.containsMouse ? "#34d399" : "#334155"
                    width: 1
                }
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        text: "💾"; font.pixelSize: 26
                        color: exportMouse.containsMouse ? "#34d399" : "#cbd5e1"
                    }
                    Text {
                        text: "导出结果"; font.pixelSize: 18
                        color: exportMouse.containsMouse ? "#34d399" : "#cbd5e1"
                    }
                }
                MouseArea {
                    id: exportMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: console.log("导出结果")
                }
            }
        }
    }
}
