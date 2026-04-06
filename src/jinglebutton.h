#pragma once
#include <QPushButton>
#include <QColor>

class JingleButton : public QPushButton
{
    Q_OBJECT
public:
    explicit JingleButton(int index, QWidget *parent = nullptr);

    int  index() const { return m_index; }
    void setPlaying(bool on);
    void setPaused(bool on);
    void setLoopIndicatorVisible(bool v);
    bool loopIndicatorVisible() const { return m_showLoop; }
    void setBaseColor(const QColor &c);
    void setDurationText(const QString &t) { m_durationText = t; update(); }

signals:
    void rightClicked(int index);

protected:
    void contextMenuEvent(QContextMenuEvent *) override;
    void paintEvent(QPaintEvent *) override;

private:
    int    m_index;
    bool   m_playing  = false;
    bool   m_paused   = false;
    bool   m_showLoop = false;
    QColor m_baseColor;
    QString m_durationText;
};
