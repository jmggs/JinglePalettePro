#pragma once
#include <QString>
#include <QColor>
#include <QSettings>

class SettingsManager {
public:
    explicit SettingsManager(const QString &iniPath);

    // Loads all settings from file, applying defaults if missing
    void load();
    // Saves all settings to file
    void save();
    // Apply default values (first run)
    void applyDefaults(const QString &appPath);

    // ---- Display ----
    QString timeFormat;
    QString dateFormat;
    int     remainWarn;
    QColor  remainColor;
    int     refreshInterval;   // ms
    int     autoMixTime;       // ms

    // ---- Audio ----
    int     device;
    int     volume;            // 0..100

    // ---- Others ----
    QString language;
    bool    errorLog;
    bool    alwaysOnTop;
    int     threadPriority;

    // ---- Window state ----
    int     winWidth;
    int     winHeight;
    int     winState;          // 0=normal,1=minimised,2=maximised
    int     posTop;
    int     posLeft;

    // ---- Tabs ----
    int     activeTab;

    // ---- Misc ----
    int     touch;
    int     autoRepeat;
    int     autoMix;

    // ---- Time Announcer ----
    QString timeAnnouncer;
    int     timeAnnMin;
    int     timeAnnSec;
    int     timeAnnDel;

    // ---- Palette ----
    QString paletteIndex;

    QString iniPath() const { return m_iniPath; }

private:
    QString   m_iniPath;
    QSettings *m_s = nullptr;
};
