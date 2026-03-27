#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QString>
#include <QUrl>
#include <array>
#include "globals.h"

class AudioEngine : public QObject
{
    Q_OBJECT
public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    // Initialise audio device (index: 0=default, 1..N=specific)
    bool init(int deviceIndex = 0);
    void free();

    // Master volume 0..100
    void setMasterVolume(int vol);
    int  masterVolume() const { return m_masterVol; }

    // --- Jingle operations ---
    bool loadJingle(int idx, const QString &path);
    void playJingle(int idx);
    void stopJingle(int idx);
    void pauseJingle(int idx);
    void resumeJingle(int idx);
    void seekJingle(int idx, qint64 posMs = 0);
    void setJingleVolume(int idx, int vol);  // 0..100
    void setJingleLoop(int idx, bool loop);
    bool isPlaying(int idx) const;
    bool isPaused(int idx) const;
    bool isStopped(int idx) const;
    qint64 jingleDurationMs(int idx) const;
    qint64 jinglePositionMs(int idx) const;

    // Get current VU level 0..100
    float vuLevelLeft()  const { return m_vuLeft; }
    float vuLevelRight() const { return m_vuRight; }

    // --- Stream (internet radio / remote file) ---
    bool  playStream(const QUrl &url);
    void  stopStream();
    bool  isStreamPlaying() const;

    // --- Time Announce ---
    bool  loadTimeAnnounce(const QString &soundPath, const QString &jinglePath);
    void  playTimeAnnounce();
    void  stopTimeAnnounce();
    bool  isTimeAnnouncePlaying() const;

    // --- Auto-mix: fade out all except keepIdx over durationMs ---
    void  startAutoMix(int keepIdx, int durationMs);

    // Available audio output devices
    static QStringList availableDevices();

    JingleData jings[JINGLE_COUNT];

signals:
    void jingleFinished(int idx);
    void streamStatusChanged(const QString &status);
    void vuLevelsUpdated(float left, float right);
    void timeAnnounceFinished();

private slots:
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void onStreamStateChanged(QMediaPlayer::PlaybackState state);
    void onStreamError(QMediaPlayer::Error error, const QString &msg);
    void onVuTimer();
    void onAutoMixTimer();

private:
    void createPlayer(int idx);
    void updateVuLevels();

    int  m_masterVol = 100;
    float m_vuLeft   = 0.f;
    float m_vuRight  = 0.f;

    // Stream player
    QMediaPlayer  *m_streamPlayer  = nullptr;
    QAudioOutput  *m_streamOut     = nullptr;

    // Time announce players
    QMediaPlayer  *m_taPlayer1     = nullptr;  // the beep/sound
    QAudioOutput  *m_taOut1        = nullptr;
    QMediaPlayer  *m_taPlayer2     = nullptr;  // the jingle
    QAudioOutput  *m_taOut2        = nullptr;

    // VU meter update timer
    QTimer        *m_vuTimer       = nullptr;

    // Auto-mix timer
    QTimer        *m_mixTimer      = nullptr;
    int            m_mixKeepIdx    = -1;
    int            m_mixStep       = 0;
    int            m_mixSteps      = 100;
    std::array<int,JINGLE_COUNT> m_mixStartVol = {};

    // decay for VU
    float          m_vuDecayL      = 0.f;
    float          m_vuDecayR      = 0.f;
};
