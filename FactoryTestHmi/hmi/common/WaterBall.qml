import QtQuick

Canvas {
    id: ball

    property real percent: 0         // 0~100
    property alias label: title.text
    property color waterColor: "#60a5fa"
    property color bgColor: "#1e293b"
    property real animPercent: 0

    Behavior on animPercent { NumberAnimation { duration: 600; easing.type: Easing.OutCubic } }
    onPercentChanged: animPercent = percent

    width: 200; height: 200
    onAnimPercentChanged: requestPaint()
    onWidthChanged: requestPaint()
    Component.onCompleted: requestPaint()

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 4
        Text {
            id: title
            anchors.horizontalCenter: parent.horizontalCenter
            font.family: "Inter, Segoe UI, sans-serif"
            font.pixelSize: 14; font.weight: Font.Bold
            color: "#94a3b8"
        }
    }

    onPaint: {
        var ctx = getContext("2d")
        var w = width, h = height, cx = w / 2, cy = h / 2 - 8, r = Math.min(w, h - 20) / 2 - 4
        ctx.clearRect(0, 0, w, h)

        // 背景圆
        ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2)
        ctx.fillStyle = bgColor; ctx.fill()

        // 水球裁剪区域
        ctx.save()
        ctx.beginPath(); ctx.arc(cx, cy, r - 2, 0, Math.PI * 2)
        ctx.clip()

        // 水位高度
        var level = (h - 16 - 2 * r) + 2 * r * (1 - animPercent / 100)

        // 水底色
        ctx.fillStyle = Qt.rgba(waterColor.r, waterColor.g, waterColor.b, 0.35)
        ctx.fillRect(cx - r, level, 2 * r, h)

        // 波形（3 条正弦叠加）
        ctx.fillStyle = waterColor
        ctx.beginPath()
        ctx.moveTo(cx - r, h)
        var amp = 3, freq = 3, time = Date.now() / 1000
        for (var x = cx - r; x <= cx + r; x++) {
            var wave = Math.sin(x * 0.04 + time * 2) * amp
                     + Math.sin(x * 0.07 - time * 1.5) * amp * 0.6
                     + Math.sin(x * 0.11 + time * 1.2) * amp * 0.3
            ctx.lineTo(x, level + wave)
        }
        ctx.lineTo(cx + r, h)
        ctx.closePath(); ctx.fill()

        ctx.restore()

        // 外环
        ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2)
        ctx.strokeStyle = waterColor; ctx.lineWidth = 2.5; ctx.stroke()

        // 百分比文字
        ctx.fillStyle = "#e2e8f0"
        ctx.font = "bold 28px Inter, Segoe UI, sans-serif"; ctx.textAlign = "center"
        ctx.fillText(animPercent.toFixed(0) + "%", cx, cy + 8)
    }

    Timer {
        interval: 50; running: true; repeat: true
        onTriggered: ball.requestPaint()
    }
}
