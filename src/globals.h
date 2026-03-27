#pragma once
#include <QString>
#include <QColor>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <memory>

// Number of jingle buttons
constexpr int JINGLE_COUNT = 30;

// Jingle data structure (mirrors VB6 JingCheck type)
struct JingleData {
    QString path;
    QColor  color       = QColor(0xF0, 0xF0, 0xF0);   // vbButtonFace
    bool    onAir       = false;
    bool    paused      = false;
    long    volume      = 100;    // 0..100
    bool    loop        = false;
    float   vuL         = 0.f;
    float   vuR         = 0.f;
    bool    inDebt      = false;  // needs reload after current play
    std::shared_ptr<QMediaPlayer>  player;
    std::shared_ptr<QAudioOutput>  audioOut;
};
