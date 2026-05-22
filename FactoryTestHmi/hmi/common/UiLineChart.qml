import QtQuick

Canvas {
    id: chart

    property var series: ({})
    property var colors: ({})
    property var labels: ({})
    property real lineWidth: 1.5
    property color gridColor: "#1e293b"
    property int windowSec: 120

    property real padL: 68; property real padR: 36
    property real padT: 12; property real padB: 34
    property int xTicks: 20

    onSeriesChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Component.onCompleted: requestPaint()

    function fmtTime(secsAgo) {
        var d = new Date(Date.now() - secsAgo * 1000)
        var hh = ("0" + d.getHours()).slice(-2)
        var mm = ("0" + d.getMinutes()).slice(-2)
        var ss = ("0" + d.getSeconds()).slice(-2)
        return hh + ":" + mm + ":" + ss
    }

    onPaint: {
        var ctx = getContext("2d")
        var w = width, h = height
        ctx.clearRect(0, 0, w, h)
        ctx.fillStyle = "#0f172a"
        ctx.fillRect(0, 0, w, h)

        var pw = w - padL - padR, ph = h - padT - padB
        if (pw <= 0 || ph <= 0) return

        // ===== Y 轴：10 ~ 130，每 5 度 =====
        var yMin = 10, yMax = 130, yStep = 5
        var ySteps = (yMax - yMin) / yStep
        ctx.strokeStyle = gridColor; ctx.lineWidth = 0.5
        ctx.fillStyle = "#64748b"; ctx.font = "10px Inter, Segoe UI, sans-serif"; ctx.textAlign = "right"
        for (var yi = 0; yi <= ySteps; yi++) {
            var val = yMin + yi * yStep
            var y = padT + ph * (yMax - val) / (yMax - yMin)
            ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(w - padR, y); ctx.stroke()
            ctx.fillText(val, padL - 6, y + 3)
        }

        // ===== 最大数据长度 =====
        var keys = Object.keys(series)
        var maxLen = 2
        for (var ki = 0; ki < keys.length; ki++) {
            var al = series[keys[ki]] ? series[keys[ki]].length : 0
            if (al > maxLen) maxLen = al
        }
        var win = maxLen > windowSec ? windowSec : maxLen
        if (win < 2) win = 2
        var secPerTick = win / (xTicks - 1)

        // 横轴：固定 20 个位置，hh:mm:ss，12px 字体
        ctx.fillStyle = "#64748b"; ctx.font = "12px Inter, Segoe UI, sans-serif"; ctx.textAlign = "center"
        for (var ti = 0; ti < xTicks; ti++) {
            var sec = Math.round(ti * secPerTick)
            var tx = padL + pw - pw * ti / (xTicks - 1)
            ctx.fillText(fmtTime(sec), tx, h - 4)
        }

        // ===== 曲线 =====
        for (ki = 0; ki < keys.length; ki++) {
            var key = keys[ki], d = series[key]
            if (!d || d.length < 1) continue
            ctx.strokeStyle = colors[key] || "#60a5fa"
            ctx.lineWidth = lineWidth; ctx.lineJoin = "round"
            ctx.beginPath()
            var first = true
            for (var di = 0; di < d.length; di++) {
                var offset = d.length - 1 - di
                if (offset > win) continue
                var px = padL + pw - pw * offset / win
                var py = padT + ph * (yMax - d[di]) / (yMax - yMin)
                if (py < padT) py = padT; if (py > padT + ph) py = padT + ph
                if (first) { ctx.moveTo(px, py); first = false }
                else ctx.lineTo(px, py)
            }
            ctx.stroke()
        }

        // ===== 图例：右对齐 =====
        ctx.font = "bold 20px Inter, Segoe UI, sans-serif"; ctx.textAlign = "right"
        var rx = w - padR
        for (ki = keys.length - 1; ki >= 0; ki--) {
            var lk = keys[ki], lc = colors[lk] || "#60a5fa", ll = labels[lk] || lk
            var tw = ctx.measureText(ll).width
            ctx.fillStyle = lc; ctx.fillRect(rx - tw - 38, 6, 28, 5)
            ctx.fillStyle = "#94a3b8"; ctx.fillText(ll, rx, 6 + 14)
            rx -= tw + 46
        }
    }
}
