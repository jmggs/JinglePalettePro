#include "audioengine.h"
#include "errorlogger.h"
#include <QMediaDevices>
#include <QAudioDevice>
#include <QTimer>
#include <QtMath>
#include <QFileInfo>
#include <cmath>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
{
    // Initialise jingle players
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        createPlayer(i);
    }

    // VU meter timer - 50ms refresh
    m_vuTimer = new QTimer(this);
    m_vuTimer->setInterval(50);
    connect(m_vuTimer, &QTimer::timeout, this, &AudioEngine::onVuTimer);
    m_vuTimer->start();

    // Auto-mix timer
    m_mixTimer = new QTimer(this);
    m_mixTimer->setInterval(30);
    connect(m_mixTimer, &QTimer::timeout, this, &AudioEngine::onAutoMixTimer);

    // Stream player
    m_streamPlayer = new QMediaPlayer(this);
    m_streamOut    = new QAudioOutput(this);
    m_streamPlayer->setAudioOutput(m_streamOut);
    connect(m_streamPlayer, &QMediaPlayer::playbackStateChanged,
            this, &AudioEngine::onStreamStateChanged);
    connect(m_streamPlayer, &QMediaPlayer::errorOccurred,
            this, &AudioEngine::onStreamError);

    // Time announce players
    m_taPlayer1 = new QMediaPlayer(this);
    m_taOut1    = new QAudioOutput(this);
    m_taPlayer1->setAudioOutput(m_taOut1);

    m_taPlayer2 = new QMediaPlayer(this);
    m_taOut2    = new QAudioOutput(this);
    m_taPlayer2->setAudioOutput(m_taOut2);

        // PCM level decoder (pre-computa envelope RMS ao carregar jingle)
    m_levelDecoder = new QAudioDecoder(this);
    connect(m_levelDecoder, &QAudioDecoder::bufferReady,
            this, &AudioEngine::onLevelDecoderBuffer);
    connect(m_levelDecoder, &QAudioDecoder::finished,
            this, &AudioEngine::onLevelDecoderFinished);
            connect(m_levelDecoder,
                static_cast<void (QAudioDecoder::*)(QAudioDecoder::Error)>(&QAudioDecoder::error),
                this, &AudioEngine::onLevelDecoderError);
}

AudioEngine::~AudioEngine()
{
    free();
}

bool AudioEngine::init(int deviceIndex)
{
    const auto devices = QMediaDevices::audioOutputs();
    QAudioDevice dev;
    if (deviceIndex > 0 && deviceIndex <= devices.size())
        dev = devices.at(deviceIndex - 1);
    else
        dev = QMediaDevices::defaultAudioOutput();

    // Apply to all outputs
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (jings[i].audioOut) {
            jings[i].audioOut->setDevice(dev);
        }
    }
    if (m_streamOut) m_streamOut->setDevice(dev);
    if (m_taOut1)    m_taOut1->setDevice(dev);
    if (m_taOut2)    m_taOut2->setDevice(dev);

    setMasterVolume(m_masterVol);
    return true;
}

void AudioEngine::free()
{
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (jings[i].player) {
            jings[i].player->stop();
        }
    }
    if (m_streamPlayer) m_streamPlayer->stop();
    if (m_taPlayer1)    m_taPlayer1->stop();
    if (m_taPlayer2)    m_taPlayer2->stop();
}

void AudioEngine::createPlayer(int idx)
{
    auto *player = new QMediaPlayer(this);
    auto *out    = new QAudioOutput(this);
    player->setAudioOutput(out);
    out->setVolume(1.0f);

    // Track which player finished
    connect(player, &QMediaPlayer::playbackStateChanged, this,
            [this, idx](QMediaPlayer::PlaybackState state) {
                if (state == QMediaPlayer::StoppedState) {
                    // Only emit finished if it was playing (not a deliberate stop)
                    if (jings[idx].onAir) {
                        jings[idx].onAir = false;
                        jings[idx].vuL   = 0;
                        jings[idx].vuR   = 0;
                        emit jingleFinished(idx);
                        // Reload if inDebt
                        if (jings[idx].inDebt) {
                            jings[idx].inDebt = false;
                            loadJingle(idx, jings[idx].path);
                        }
                    }
                }
            });

    jings[idx].player   = std::shared_ptr<QMediaPlayer>(player, [](QMediaPlayer*){});
    jings[idx].audioOut = std::shared_ptr<QAudioOutput>(out, [](QAudioOutput*){});
}

void AudioEngine::setMasterVolume(int vol)
{
    m_masterVol = vol;
    float v = vol / 100.f;
    if (m_streamOut) m_streamOut->setVolume(v);
    if (m_taOut1)    m_taOut1->setVolume(v);
    if (m_taOut2)    m_taOut2->setVolume(v);
    // Individual jingle volumes are blended with master
    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (jings[i].audioOut) {
            float jv = (jings[i].volume / 100.f) * v;
            jings[i].audioOut->setVolume(jv);
        }
    }
}

bool AudioEngine::loadJingle(int idx, const QString &path)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return false;
    jings[idx].path = path;
    if (path.isEmpty()) {
        jings[idx].player->setSource(QUrl());
        return true;
    }
    QUrl url = QUrl::fromLocalFile(path);
    jings[idx].player->setSource(url);

    // Pré-computa envelope de nível PCM real (via fila, sem cancelar decode activo)
    jings[idx].levelEnvL.clear();
    jings[idx].levelEnvR.clear();
    if (!path.isEmpty()) {
        // Remove entrada anterior para este índice (evita duplicados na fila)
        QQueue<QPair<int,QString>> filtered;
        while (!m_decodeQueue.isEmpty()) {
            auto item = m_decodeQueue.dequeue();
            if (item.first != idx) filtered.enqueue(item);
        }
        m_decodeQueue = filtered;
        m_decodeQueue.enqueue({idx, path});
        if (!m_levelDecBusy)
            startNextDecode();
    }

    jings[idx].onAir  = false;
    jings[idx].paused = false;
    return true;
}

void AudioEngine::playJingle(int idx)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return;
    auto *p = jings[idx].player.get();
    if (!p) return;

    // Apply volume
    float jv = (jings[idx].volume / 100.f) * (m_masterVol / 100.f);
    jings[idx].audioOut->setVolume(jv);

    if (p->playbackState() == QMediaPlayer::PlayingState) {
        // Already playing: if autoRepeat seek to start, else stop
        p->setPosition(0);
    } else {
        p->setPosition(0);
        p->play();
        jings[idx].onAir  = true;
        jings[idx].paused = false;
    }
}

void AudioEngine::stopJingle(int idx)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return;
    jings[idx].onAir  = false;
    jings[idx].paused = false;
    jings[idx].player->stop();
    jings[idx].player->setPosition(0);
}

void AudioEngine::pauseJingle(int idx)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return;
    jings[idx].player->pause();
    jings[idx].paused = true;
}

void AudioEngine::resumeJingle(int idx)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return;
    jings[idx].player->play();
    jings[idx].paused = false;
}

void AudioEngine::seekJingle(int idx, qint64 posMs)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return;
    jings[idx].player->setPosition(posMs);
}

void AudioEngine::setJingleVolume(int idx, int vol)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return;
    jings[idx].volume = vol;
    float jv = (vol / 100.f) * (m_masterVol / 100.f);
    jings[idx].audioOut->setVolume(jv);
}

void AudioEngine::setJingleLoop(int idx, bool loop)
{
    if (idx < 0 || idx >= JINGLE_COUNT) return;
    jings[idx].loop = loop;
    jings[idx].player->setLoops(loop ? QMediaPlayer::Infinite : 1);
}

bool AudioEngine::isPlaying(int idx) const
{
    if (idx < 0 || idx >= JINGLE_COUNT) return false;
    return jings[idx].player->playbackState() == QMediaPlayer::PlayingState;
}

bool AudioEngine::isPaused(int idx) const
{
    if (idx < 0 || idx >= JINGLE_COUNT) return false;
    return jings[idx].player->playbackState() == QMediaPlayer::PausedState;
}

bool AudioEngine::isStopped(int idx) const
{
    if (idx < 0 || idx >= JINGLE_COUNT) return false;
    return jings[idx].player->playbackState() == QMediaPlayer::StoppedState;
}

qint64 AudioEngine::jingleDurationMs(int idx) const
{
    if (idx < 0 || idx >= JINGLE_COUNT) return 0;
    return jings[idx].player->duration();
}

qint64 AudioEngine::jinglePositionMs(int idx) const
{
    if (idx < 0 || idx >= JINGLE_COUNT) return 0;
    return jings[idx].player->position();
}

// --- Stream ---
bool AudioEngine::playStream(const QUrl &url)
{
    m_streamPlayer->stop();
    m_streamPlayer->setSource(url);
    m_streamOut->setVolume(m_masterVol / 100.f);
    m_streamPlayer->play();
    return true;
}

void AudioEngine::stopStream()
{
    m_streamPlayer->stop();
}

bool AudioEngine::isStreamPlaying() const
{
    return m_streamPlayer->playbackState() == QMediaPlayer::PlayingState;
}

// --- Time Announce ---
bool AudioEngine::loadTimeAnnounce(const QString &soundPath, const QString &jinglePath)
{
    if (!soundPath.isEmpty())
        m_taPlayer1->setSource(QUrl::fromLocalFile(soundPath));
    if (!jinglePath.isEmpty())
        m_taPlayer2->setSource(QUrl::fromLocalFile(jinglePath));
    return true;
}

void AudioEngine::playTimeAnnounce()
{
    m_taOut1->setVolume(m_masterVol / 100.f);
    m_taPlayer1->setPosition(0);
    m_taPlayer1->play();
}

void AudioEngine::stopTimeAnnounce()
{
    m_taPlayer1->stop();
    m_taPlayer2->stop();
}

bool AudioEngine::isTimeAnnouncePlaying() const
{
    return m_taPlayer1->playbackState() == QMediaPlayer::PlayingState
        || m_taPlayer2->playbackState() == QMediaPlayer::PlayingState;
}

// --- Auto-mix ---
void AudioEngine::startAutoMix(int keepIdx, int durationMs)
{
    if (!m_mixTimer) return;
    m_mixKeepIdx = keepIdx;
    m_mixStep    = 0;
    m_mixSteps   = qMax(1, durationMs / 30);
    for (int i = 0; i < JINGLE_COUNT; ++i)
        m_mixStartVol[i] = jings[i].volume;
    m_mixTimer->start();
}

void AudioEngine::onAutoMixTimer()
{
    m_mixStep++;
    float progress = (float)m_mixStep / m_mixSteps;
    float fade     = 1.f - progress;

    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (i == m_mixKeepIdx) continue;
        if (!isPlaying(i)) continue;
        float newVol = m_mixStartVol[i] * fade;
        float jv = (newVol / 100.f) * (m_masterVol / 100.f);
        jings[i].audioOut->setVolume(qMax(0.f, jv));
    }

    if (m_mixStep >= m_mixSteps) {
        // Stop faded channels, restore volumes
        for (int i = 0; i < JINGLE_COUNT; ++i) {
            if (i == m_mixKeepIdx) continue;
            if (isPlaying(i)) stopJingle(i);
            setJingleVolume(i, m_mixStartVol[i]);
        }
        m_mixTimer->stop();
    }
}

// --- VU levels (approximation using playback state) ---
void AudioEngine::onVuTimer()
{
    updateVuLevels();
    emit vuLevelsUpdated(m_vuLeft, m_vuRight);
}

void AudioEngine::onLevelDecoderBuffer()
{
    if (m_levelDecIdx < 0 || !m_levelDecoder->bufferAvailable()) return;
    QAudioBuffer buf = m_levelDecoder->read();
    if (!buf.isValid()) return;

    const QAudioFormat &fmt   = buf.format();
    const int ch              = fmt.channelCount();
    const int frames          = buf.frameCount();
    const int sr              = fmt.sampleRate();
    const int blockFrames     = sr * 50 / 1000; // frames por bloco de 50ms
    const auto sampleFmt      = fmt.sampleFormat();

    for (int f = 0; f < frames; ++f) {
        float sL = 0.f, sR = 0.f;

        if (sampleFmt == QAudioFormat::Float) {
            const float *data = buf.constData<float>();
            sL = data[f * ch];
            sR = (ch >= 2) ? data[f * ch + 1] : sL;
        } else if (sampleFmt == QAudioFormat::Int16) {
            const qint16 *data = buf.constData<qint16>();
            sL = data[f * ch] / 32768.f;
            sR = (ch >= 2) ? data[f * ch + 1] / 32768.f : sL;
        } else if (sampleFmt == QAudioFormat::Int32) {
            const qint32 *data = buf.constData<qint32>();
            sL = data[f * ch] / 2147483648.f;
            sR = (ch >= 2) ? data[f * ch + 1] / 2147483648.f : sL;
        } else if (sampleFmt == QAudioFormat::UInt8) {
            const quint8 *data = buf.constData<quint8>();
            sL = (data[f * ch] - 128) / 128.f;
            sR = (ch >= 2) ? (data[f * ch + 1] - 128) / 128.f : sL;
        } else {
            continue;
        }

        m_levelDecSumL += sL * sL;
        m_levelDecSumR += sR * sR;

        ++m_levelDecFrameCnt;

        if (m_levelDecFrameCnt >= blockFrames) {
            jings[m_levelDecIdx].levelEnvL.append(
                qSqrt(m_levelDecSumL / m_levelDecFrameCnt));
            jings[m_levelDecIdx].levelEnvR.append(
                qSqrt(m_levelDecSumR / m_levelDecFrameCnt));
            m_levelDecSumL     = 0.f;
            m_levelDecSumR     = 0.f;
            m_levelDecFrameCnt = 0;
        }
    }
}

void AudioEngine::onLevelDecoderFinished()
{
    // Flush bloco parcial final
    if (m_levelDecIdx >= 0 && m_levelDecFrameCnt > 0) {
        jings[m_levelDecIdx].levelEnvL.append(
            qSqrt(m_levelDecSumL / m_levelDecFrameCnt));
        jings[m_levelDecIdx].levelEnvR.append(
            qSqrt(m_levelDecSumR / m_levelDecFrameCnt));
    }
    m_levelDecIdx      = -1;
    m_levelDecFrameCnt = 0;
    m_levelDecSumL     = 0.f;
    m_levelDecSumR     = 0.f;
    m_levelDecBusy     = false;
    m_levelDecoder->stop();

    // Processar próximo item da fila
    startNextDecode();
}

void AudioEngine::startNextDecode()
{
    if (m_decodeQueue.isEmpty()) return;
    auto [idx, path] = m_decodeQueue.dequeue();
    m_levelDecIdx      = idx;
    m_levelDecFrameCnt = 0;
    m_levelDecSumL     = 0.f;
    m_levelDecSumR     = 0.f;
    m_levelDecBusy     = true;
    m_levelDecoder->stop();
    m_levelDecoder->setSource(QUrl::fromLocalFile(path));
    m_levelDecoder->start();
    qDebug() << "[VU] decode start idx=" << idx << path;
}

void AudioEngine::onLevelDecoderError(QAudioDecoder::Error err)
{
    qDebug() << "[VU] decode ERROR idx=" << m_levelDecIdx
             << "error=" << err
             << m_levelDecoder->errorString();
    m_levelDecoder->stop();
    m_levelDecFrameCnt = 0;
    m_levelDecSumL     = 0.f;
    m_levelDecSumR     = 0.f;
    m_levelDecBusy = false;
    m_levelDecIdx  = -1;
    startNextDecode();
}

void AudioEngine::updateVuLevels()
{
    float maxL = 0.f, maxR = 0.f;

    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (!isPlaying(i)) continue;

        const qint64 posMs = jings[i].player->position();
        const int    block = static_cast<int>(posMs / 50);
        float lvL = 0.f, lvR = 0.f;

        if (block >= 0 && block < jings[i].levelEnvL.size()) {
            // PCM real: RMS do bloco correspondente à posição actual
            lvL = jings[i].levelEnvL[block];
            lvR = jings[i].levelEnvR[block];
        } else {
            // Envelope ainda não calculado: não mostrar nada
            continue;
        }

        const float volScale = jings[i].volume / 100.f;
        maxL = qMax(maxL, lvL * volScale);
        maxR = qMax(maxR, lvR * volScale);
    }

    if (isStreamPlaying()) {
        maxL = qMax(maxL, 0.60f);
        maxR = qMax(maxR, 0.60f);
    }
    if (isTimeAnnouncePlaying()) {
        maxL = qMax(maxL, 0.65f);
        maxR = qMax(maxR, 0.65f);
    }

    // Converte amplitude linear RMS (0..1) → dBFS → escala VuMeter (0..100)
    // VuMeter: 0 = -60 dBFS, 100 = 0 dBFS
    constexpr float DB_MIN = -60.f;
    auto toVu = [DB_MIN](float rms) -> float {
        if (rms < 1e-6f) return 0.f;
        float db = 20.f * std::log10(rms);
        if (db < DB_MIN) db = DB_MIN;
        if (db > 0.f)    db = 0.f;
        return (db - DB_MIN) / (-DB_MIN) * 100.f;
    };

    m_vuLeft  = toVu(maxL);
    m_vuRight = toVu(maxR);
}

void AudioEngine::onStreamStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        emit streamStatusChanged("Playing");
        break;
    case QMediaPlayer::PausedState:
        emit streamStatusChanged("Paused");
        break;
    case QMediaPlayer::StoppedState:
        emit streamStatusChanged("Stopped");
        break;
    }
}

void AudioEngine::onStreamError(QMediaPlayer::Error /*error*/, const QString &msg)
{
    emit streamStatusChanged("Error: " + msg);
    ErrorLogger::instance().log("Stream", msg);
}

void AudioEngine::onPlayerStateChanged(QMediaPlayer::PlaybackState /*state*/)
{
    // handled per-player in lambda in createPlayer
}

QStringList AudioEngine::availableDevices()
{
    QStringList names;
    names << "Default";
    for (const auto &d : QMediaDevices::audioOutputs())
        names << d.description();
    return names;
}
