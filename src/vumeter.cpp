#include "vumeter.h"
#include <QPainter>
#include <QTimerEvent>
#include <QLinearGradient>

VuMeter::VuMeter(Qt::Orientation orient, QWidget *parent)
    : QWidget(parent), m_orient(orient)
{
    setMinimumSize(12, 40);
    m_peakTimer = startTimer(50);
}

void VuMeter::setLevel(float level)
{
    m_level = qBound(0.f, level, 100.f);
    if (m_level > m_peak) {
        m_peak      = m_level;
        m_peakCount = 0;
    }
    update();
}

void VuMeter::timerEvent(QTimerEvent *)
{
    if (m_peak > 0.f) {
        m_peakCount++;
        if (m_peakCount > m_peakDelay / 50) {
            m_peak = qMax(0.f, m_peak - 1.5f);
            update();
        }
    }
    // Decay level visually
    if (m_level > 0.f) {
        m_level = qMax(0.f, m_level - 3.f);
        update();
    }
}

void VuMeter::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int W = width();
    const int H = height();
    p.fillRect(rect(), QColor("#222222"));

    // --- VU vertical minimalista, sem texto, segmentos finos, ocupa toda a área ---
    const float min_db = -60.0f;
    const float max_db = 0.0f;
    const int segment_gap = 1;
    const int segment_count = 144;
    const int label_height = 16;
    const int bar_top = 2;
    const int bar_bottom = label_height + 2;
    const int bar_left = 2;
    const int bar_right = 2;
    const int bar_width = W - bar_left - bar_right;
    const int bar_height = H - bar_top - bar_bottom;
    const int segment_height = std::max(1, (bar_height - (segment_count - 1) * segment_gap) / segment_count);
    const int segment_width = bar_width;

    auto segment_color = [](float db) -> QColor {
        if (db >= -8)   return QColor("#ff0000");
        if (db >= -12)  return QColor("#ff6600");
        if (db >= -20)  return QColor("#ffff00");
        if (db >= -40)  return QColor("#00ff00");
        return QColor("#66ff00");
    };

    float vu_db = min_db + (max_db - min_db) * (m_level / 100.f);
    int filled = int((vu_db - min_db) / (max_db - min_db) * segment_count + 0.5f);
    for (int i = 0; i < segment_count; ++i) {
        float db = min_db + (max_db - min_db) * (i / float(segment_count - 1));
        int y = H - bar_bottom - (i + 1) * (segment_height + segment_gap) + segment_gap;
        QRect segRect(bar_left, y, segment_width, segment_height);
        QColor color = (i < filled) ? segment_color(db) : QColor("#222222");
        p.fillRect(segRect, color);
        // Borda discreta: linhas esquerda e inferior
        p.setPen(QPen(QColor("#333333"), 1));
        p.drawLine(segRect.topLeft(), segRect.bottomLeft()); // esquerda
        p.drawLine(segRect.bottomLeft(), segRect.bottomRight()); // baixo
    }

    // L/R em baixo, centralizado
    QFont fch = font(); fch.setPointSize(11); fch.setBold(true); p.setFont(fch);
    QString ch;
    if (objectName().endsWith("L", Qt::CaseInsensitive))
        ch = "L";
    else if (objectName().endsWith("R", Qt::CaseInsensitive))
        ch = "R";
    else
        ch = "?";
    p.setPen(QPen(QColor("#eeeeee")));
    p.drawText(0, H - label_height, W, label_height, Qt::AlignHCenter | Qt::AlignVCenter, ch);

    // Border
    p.setPen(QPen(QColor(34, 34, 34), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(0, 0, W - 1, H - 1);
}
