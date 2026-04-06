#include "jinglebutton.h"
#include <QPainter>
#include <QContextMenuEvent>

JingleButton::JingleButton(int index, QWidget *parent)
    : QPushButton(parent), m_index(index)
{
    m_baseColor = palette().color(QPalette::Button);
    setCheckable(true);
    setMinimumHeight(80);
    setMaximumHeight(80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Cor base igual ao gridBtnStyle
    setStyleSheet("QPushButton { background: #3a3a3a; color: #ccc; font-size: 18px; font-weight: bold; border: 1px solid #555; border-radius: 4px; }"
                  "QPushButton:hover  { background: #4a4a4a; color: #fff; }"
                  "QPushButton:pressed{ background: #252525; color: #fff; }"
                  "QPushButton:checked{ background: #00bb44; color: #000; border-color: #00ee55; }");
}

void JingleButton::setPlaying(bool on)
{
    m_playing = on;
    m_paused  = false;
    if (!on) setChecked(false);
    update();
}

void JingleButton::setPaused(bool on)
{
    m_paused  = on;
    m_playing = false;
    // Não altera estilo nem fonte, mantém o padrão
    update();
}

void JingleButton::setLoopIndicatorVisible(bool v)
{
    m_showLoop = v;
    update();
}

void JingleButton::setBaseColor(const QColor &c)
{
    m_baseColor = c;
    update();
}

void JingleButton::contextMenuEvent(QContextMenuEvent *)
{
    emit rightClicked(m_index);
}

void JingleButton::paintEvent(QPaintEvent *e)
{
    QPushButton::paintEvent(e);

    QPainter p(this);

    if (!m_durationText.isEmpty()) {
        QFont f = font();
        f.setPointSize(10);
        f.setBold(false);
        p.setFont(f);
        // Green when idle, dark green when playing (green background)
        p.setPen(isChecked() ? QColor(0, 60, 10) : QColor(0, 187, 68));
        QRect r = rect().adjusted(2, 0, -2, -3);
        p.drawText(r, Qt::AlignBottom | Qt::AlignHCenter, m_durationText);
    }

    if (m_showLoop) {
        // Draw small loop indicator in top-right corner
        QRect r(width() - 10, 2, 8, 8);
        p.setBrush(QColor(0, 120, 255));
        p.setPen(Qt::NoPen);
        p.drawEllipse(r);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 5, QFont::Bold));
        p.drawText(r, Qt::AlignCenter, "L");
    }
}
