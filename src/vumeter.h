#pragma once
#include <QWidget>

class VuMeter : public QWidget
{
    Q_OBJECT
public:
    explicit VuMeter(Qt::Orientation orient = Qt::Vertical, QWidget *parent = nullptr);

    void setLevel(float level); // 0.0 .. 100.0
    float level() const { return m_level; }

    // Configurable thresholds (0..100)
    void setMidLevel(float mid)  { m_midLevel = mid; update(); }
    int  peakDelay() const       { return m_peakDelay; }

protected:
    void paintEvent(QPaintEvent *) override;
    void timerEvent(QTimerEvent *) override;

private:
    float           m_level     = 0.f;
    float           m_peak      = 0.f;
    float           m_midLevel  = 60.f;
    int             m_peakTimer = 0;
    int             m_peakDelay = 500;  // ms before peak drops
    int             m_peakCount = 0;
    Qt::Orientation m_orient;
};
