import QtQuick

Canvas {
    id: gauge

    property real freqValue: 0           // GHz 数值
    property real animValue: 0
    property color accentColor: "#a78bfa"

    property real scaleMin: 0.5; property real scaleMax: 3.0  // GHz
    property real arcStart: Math.PI * 0.75; property real arcEnd: Math.PI * 2.25

    Behavior on animValue { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }
    onFreqValueChanged: animValue = freqValue

    width: 200; height: 200
    onAnimValueChanged: requestPaint()
    onWidthChanged: requestPaint()
    Component.onCompleted: requestPaint()

    // 标签
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 4
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "DRAM 频率"
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
        ctx.fillStyle = "#1e293b"; ctx.fill()

        // ===== 渐变背景圆环（绿→黄→红） =====
        var gradSegs = 60
        for (var s = 0; s < gradSegs; s++) {
            var t = s / (gradSegs - 1)  // 0~1, 0.5GHz→3.0GHz
            var a1 = arcStart + (arcEnd - arcStart) * s / gradSegs
            var a2 = arcStart + (arcEnd - arcStart) * (s + 1) / gradSegs
            // 绿 #22c55e → 黄 #eab308 → 红 #ef4444
            var rr, gg, bb
            if (t < 0.5) {
                var tt = t * 2
                rr = Math.round(34 + tt * (234 - 34))
                gg = Math.round(197 + tt * (179 - 197))
                bb = Math.round(94 + tt * (8 - 94))
            } else {
                var tt2 = (t - 0.5) * 2
                rr = Math.round(234 + tt2 * (239 - 234))
                gg = Math.round(179 + tt2 * (68 - 179))
                bb = Math.round(8 + tt2 * (68 - 8))
            }
            ctx.beginPath()
            ctx.arc(cx, cy, r, a1, a2)
            ctx.strokeStyle = "rgb(" + rr + "," + gg + "," + bb + ")"
            ctx.lineWidth = 7; ctx.lineCap = "butt"
            ctx.stroke()
        }

        // ===== 刻度 0.1 步进，全部显示标签 =====
        var tickCount = Math.round((scaleMax - scaleMin) / 0.1) + 1  // 26 ticks
        for (var i = 0; i < tickCount; i++) {
            var val = scaleMin + i * 0.1
            var ang = arcStart + (arcEnd - arcStart) * i / (tickCount - 1)
            var isHalf = Math.abs(val - Math.round(val * 2) / 2) < 0.001

            // 刻度线
            var ir = r - (isHalf ? 14 : 6), or2 = r - 2
            ctx.beginPath()
            ctx.moveTo(cx + Math.cos(ang) * ir, cy + Math.sin(ang) * ir)
            ctx.lineTo(cx + Math.cos(ang) * or2, cy + Math.sin(ang) * or2)
            ctx.strokeStyle = isHalf ? accentColor : "#475569"
            ctx.lineWidth = isHalf ? 2 : 0.5
            ctx.stroke()

            // 所有刻度显示标签
            var lr = r - (isHalf ? 22 : 14)
            var tx = cx + Math.cos(ang) * lr, ty = cy + Math.sin(ang) * lr
            ctx.fillStyle = isHalf ? "#94a3b8" : "#64748b"
            ctx.font = (isHalf ? "11px" : "9px") + " Inter, Segoe UI, sans-serif"; ctx.textAlign = "center"
            ctx.fillText(val.toFixed(1), tx, ty + 4)
        }

        // ===== 指针 =====
        var clamped = Math.max(scaleMin, Math.min(scaleMax, animValue))
        var needleAng = arcStart + (arcEnd - arcStart) * (clamped - scaleMin) / (scaleMax - scaleMin)
        var needleLen = r - 26
        ctx.beginPath()
        ctx.moveTo(cx, cy)
        ctx.lineTo(cx + Math.cos(needleAng) * needleLen, cy + Math.sin(needleAng) * needleLen)
        ctx.strokeStyle = "#ef4444"; ctx.lineWidth = 2.5; ctx.lineCap = "round"
        ctx.stroke()

        // 指针圆心
        ctx.beginPath(); ctx.arc(cx, cy, 5, 0, Math.PI * 2)
        ctx.fillStyle = "#ef4444"; ctx.fill()

        // ===== 中心数值 =====
        ctx.fillStyle = "#e2e8f0"
        ctx.font = "bold 30px Inter, Segoe UI, sans-serif"; ctx.textAlign = "center"
        ctx.fillText(animValue.toFixed(2), cx, cy + 42)

        ctx.fillStyle = "#64748b"
        ctx.font = "13px Inter, Segoe UI, sans-serif"
        ctx.fillText("GHz", cx, cy + 58)
    }
}
