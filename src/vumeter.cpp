#include "vumeter.h"
#include <QPainter>
#include <QTimerEvent>
#include <cmath>

VuMeter::VuMeter(Qt::Orientation orient, QWidget *parent)
    : QWidget(parent), m_orient(orient)
{
    setMinimumSize(12, 40);
    m_peakTimer = startTimer(50);
}

void VuMeter::setLevel(float level)
{
    m_input      = qBound(0.f, level, 100.f);
    m_hasNewInput = true;
}

void VuMeter::timerEvent(QTimerEvent *)
{
    // Ataque instantâneo (PPM); release exponencial τ=300ms
    // α = exp(-dt/τ) = exp(-50/300) ≈ 0.8465
    constexpr float DECAY = 0.8465f;

    if (m_hasNewInput) {
        if (m_input >= m_level)
            m_level = m_input;                         // ataque instantâneo
        else
            m_level = qMax(m_input, m_level * DECAY);  // decay até ao sinal, nunca abaixo
        m_hasNewInput = false;
    } else {
        m_level *= DECAY;  // sem sinal: decai até zero
    }
    if (m_level < 0.05f) m_level = 0.f;

    // Peak hold 2000 ms (AES/EBU), depois decai 1.5 dB/s (PPM Type I)
    // factor = exp(-1.5*0.05/20*ln10) = 10^(-1.5*0.05/20) ≈ 0.9914 por tick
    if (m_level > m_peak) {
        m_peak      = m_level;
        m_peakCount = 0;
    } else if (m_peak > 0.f) {
        m_peakCount++;
        if (m_peakCount > m_peakDelay / 50) {  // m_peakDelay = 2000 ms
            m_peak *= 0.9914f;
            if (m_peak < 0.05f) m_peak = 0.f;
        }
    }

    update();
}

void VuMeter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int W = width();
    const int H = height();

    p.fillRect(rect(), QColor("#111111"));

    // ── AES/EBU scale ────────────────────────────────────────────────────────
    // 60 segments, 1 per dBFS, de -60 dBFS (baixo) a 0 dBFS (cima)
    // Zonas de cor per EBU R68:
    //   Verde:       -60 .. -18 dBFS
    //   Amarelo:     -18 ..  -9 dBFS
    //   Vermelho:     -9 ..   0 dBFS
    constexpr float DB_MIN = -60.0f;
    constexpr float DB_MAX =   0.0f;
    constexpr int   N      = 120;

    constexpr int LBL_H = 16;   // área dedicada ao label L/R em baixo

    const int mx = 1;           // margem de 1px nos outros lados
    const int bW = W - 2 * mx;
    const int bH = H - LBL_H - mx; // bar vai de mx (topo) até H-LBL_H (fundo)

    // Mapeamento linear: level 0..100 → -60..0 dBFS (igual ao original)
    const float vuDb   = DB_MIN + (DB_MAX - DB_MIN) * (m_level / 100.f);
    const float peakDb = DB_MIN + (DB_MAX - DB_MIN) * (m_peak  / 100.f);

    // Fração preenchida (0..1)
    const float frac = (vuDb - DB_MIN) / (DB_MAX - DB_MIN);

    // Esquema de 5 cores do original
    auto segColor = [](float db) -> QColor {
        if (db >= -8.f)  return QColor("#ff0000");  // vermelho
        if (db >= -12.f) return QColor("#ff6600");  // laranja
        if (db >= -20.f) return QColor("#ffff00");  // amarelo
        if (db >= -40.f) return QColor("#00ff00");  // verde
        return                   QColor("#66ff00"); // lima
    };

    // Posicionamento com ponto flutuante → preenche exactamente bH
    for (int i = 0; i < N; ++i) {
        // i=0 → segmento mais baixo (-60 dBFS); i=N-1 → topo (0 dBFS)
        const float tN = float(N - 1 - i) / float(N);
        const float bN = float(N     - i) / float(N);

        const int y0 = mx + int(tN * float(bH) + 0.5f);
        const int y1 = mx + int(bN * float(bH) + 0.5f) - 1;

        if (y1 < y0) continue;

        const float db  = DB_MIN + (DB_MAX - DB_MIN) * float(i) / float(N - 1);
        const bool  lit = float(i) < frac * float(N);
        const QColor col = lit ? segColor(db) : QColor("#222222");

        p.fillRect(mx, y0, bW, y1 - y0 + 1, col);

        // Bordas discretas (igual ao original)
        p.setPen(QPen(QColor("#333333"), 1));
        p.drawLine(mx, y0, mx, y1);               // esquerda
        p.drawLine(mx, y1, mx + bW - 1, y1);      // baixo
    }

    // Peak hold: um único segmento branco no topo do fill (quando acima do nível actual)
    if (m_peak > m_level && m_peak > 0.f) {
        const float pFrac = (peakDb - DB_MIN) / (DB_MAX - DB_MIN);
        const int   pi    = qBound(0, int(pFrac * float(N - 1) + 0.5f), N - 1);

        const float tN = float(N - 1 - pi) / float(N);
        const float bN = float(N     - pi) / float(N);
        const int y0 = mx + int(tN * float(bH) + 0.5f);
        const int y1 = mx + int(bN * float(bH) + 0.5f) - 1;

        if (y1 >= y0)
            p.fillRect(mx, y0, bW, y1 - y0 + 1, QColor("#ffffff"));
    }

    // ── Label L/R em área dedicada em baixo ─────────────────────────────────
    QString ch;
    if      (objectName().endsWith("L", Qt::CaseInsensitive)) ch = "L";
    else if (objectName().endsWith("R", Qt::CaseInsensitive)) ch = "R";
    if (!ch.isEmpty()) {
        QFont f = font(); f.setPointSize(9); f.setBold(true); p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRect(0, H - LBL_H, W, LBL_H), Qt::AlignHCenter | Qt::AlignVCenter, ch);
    }

    // ── Borda ─────────────────────────────────────────────────────────────────
    p.setPen(QPen(QColor("#333333"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(0, 0, W - 1, H - 1);
}
