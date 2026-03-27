#include "jinglebutton.h"
#include <QPainter>
#include <QContextMenuEvent>

JingleButton::JingleButton(int index, QWidget *parent)
    : QPushButton(parent), m_index(index)
{
    m_baseColor = palette().color(QPalette::Button);
    setCheckable(true);
    setMinimumSize(80, 50);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setStyleSheet("text-align: center; font-size: 9px;");
}

void JingleButton::setPlaying(bool on)
{
    m_playing = on;
    m_paused  = false;
    if (on) {
        setStyleSheet("QPushButton { background-color: #1aff1a; color: black; font-size: 13px; text-align: center; border: 2px solid #006600; font-weight: bold; }");
    } else {
        setStyleSheet("QPushButton { background-color: #444; color: #eee; font-size: 13px; text-align: center; border: 2px solid #222; }");
        setChecked(false);
    }
    update();
}

void JingleButton::setPaused(bool on)
{
    m_paused  = on;
    m_playing = false;
    if (on) {
        setStyleSheet("QPushButton { background-color: #C0FFC0; font-size: 9px; text-align: center; }");
    } else {
        setStyleSheet("QPushButton { font-size: 9px; text-align: center; }");
    }
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

    if (m_showLoop) {
        QPainter p(this);
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
