#include "settingsmanager.h"
#include <QDir>
#include <QFileInfo>

SettingsManager::SettingsManager(const QString &iniPath)
    : m_iniPath(iniPath)
{
    m_s = new QSettings(iniPath, QSettings::IniFormat);
    // m_s->setIniCodec("UTF-8"); // Removido para Qt6
}

void SettingsManager::load()
{
    m_s->beginGroup("Settings");

    timeFormat      = m_s->value("TimeFormat",    "HH:mm:ss").toString();
    dateFormat      = m_s->value("DateFormat",    "yyyy MMMM d").toString();
    remainWarn      = m_s->value("RemainWarn",    3).toInt();
    remainColor     = QColor(m_s->value("RemainColor", "#0000C0").toString());
    refreshInterval = m_s->value("Refresh",       100).toInt();
    autoMixTime     = m_s->value("AutoMixTime",   1500).toInt();

    device          = m_s->value("Device",        0).toInt();
    volume          = m_s->value("Volume",        100).toInt();

    language        = m_s->value("Language",      "English").toString();
    errorLog        = m_s->value("ErrorLog",      false).toBool();
    alwaysOnTop     = m_s->value("AlwaysOnTop",   false).toBool();
    threadPriority  = m_s->value("ThreadPriority",0).toInt();

    winWidth        = m_s->value("WinWidth",      1000).toInt();
    winHeight       = m_s->value("WinHeight",     700).toInt();
    winState        = m_s->value("WinState",      0).toInt();
    posTop          = m_s->value("PosTop",        -1).toInt();
    posLeft         = m_s->value("PosLeft",       -1).toInt();

    activeTab       = m_s->value("ActiveTab",     0).toInt();
    touch           = m_s->value("Touch",         0).toInt();
    autoRepeat      = m_s->value("Autorepeat",    0).toInt();
    autoMix         = m_s->value("AutoMix",       0).toInt();

    timeAnnouncer   = m_s->value("TimeAnnouncer", "").toString();
    if (timeAnnouncer.isEmpty() || !QFileInfo::exists(timeAnnouncer)) {
        QString fallback = "/home/jomi/Downloads/JinglePalettePro-main/resources/Time_Announce.wav";
        timeAnnouncer = QFileInfo::exists(fallback) ? fallback : "";
    }
    timeAnnMin      = m_s->value("TimeAnnMin",    59).toInt();
    timeAnnSec      = m_s->value("TimeAnnSec",    55).toInt();
    timeAnnDel      = m_s->value("TimeAnnDel",    4).toInt();

    paletteIndex    = m_s->value("PaletteIndex",  "Demo").toString();

    m_s->endGroup();
}

void SettingsManager::save()
{
    m_s->beginGroup("Settings");

    m_s->setValue("TimeFormat",    timeFormat);
    m_s->setValue("DateFormat",    dateFormat);
    m_s->setValue("RemainWarn",    remainWarn);
    m_s->setValue("RemainColor",   remainColor.name());
    m_s->setValue("Refresh",       refreshInterval);
    m_s->setValue("AutoMixTime",   autoMixTime);

    m_s->setValue("Device",        device);
    m_s->setValue("Volume",        volume);

    m_s->setValue("Language",      language);
    m_s->setValue("ErrorLog",      errorLog);
    m_s->setValue("AlwaysOnTop",   alwaysOnTop);
    m_s->setValue("ThreadPriority",threadPriority);

    m_s->setValue("WinWidth",      winWidth);
    m_s->setValue("WinHeight",     winHeight);
    m_s->setValue("WinState",      winState);
    m_s->setValue("PosTop",        posTop);
    m_s->setValue("PosLeft",       posLeft);

    m_s->setValue("ActiveTab",     activeTab);
    m_s->setValue("Touch",         touch);
    m_s->setValue("Autorepeat",    autoRepeat);
    m_s->setValue("AutoMix",       autoMix);

    m_s->setValue("TimeAnnouncer", timeAnnouncer);
    m_s->setValue("TimeAnnMin",    timeAnnMin);
    m_s->setValue("TimeAnnSec",    timeAnnSec);
    m_s->setValue("TimeAnnDel",    timeAnnDel);

    m_s->setValue("PaletteIndex",  paletteIndex);

    m_s->endGroup();
    m_s->sync();
}

void SettingsManager::applyDefaults(const QString &appPath)
{
    timeFormat      = "HH:mm:ss";
    dateFormat      = "yyyy MMMM d";
    remainWarn      = 3;
    remainColor     = QColor(0, 0, 192);
    refreshInterval = 100;
    autoMixTime     = 1500;

    device          = 0;
    volume          = 100;

    language        = "English";
    errorLog        = false;
    alwaysOnTop     = false;
    threadPriority  = 0;

    winWidth        = 1000;
    winHeight       = 700;
    winState        = 0;
    posTop          = -1;
    posLeft         = -1;

    activeTab       = 0;
    touch           = 0;
    autoRepeat      = 0;
    autoMix         = 0;

    QString wav = "/home/jomi/Downloads/JinglePalettePro-main/resources/Time_Announce.wav";
    if (!QFileInfo::exists(wav))
        wav = appPath + "/Time_Announce.wav";
    timeAnnouncer   = QFileInfo::exists(wav) ? wav : "";
    timeAnnMin      = 59;
    timeAnnSec      = 55;
    timeAnnDel      = 4;

    paletteIndex    = "Demo";

    save();
}
