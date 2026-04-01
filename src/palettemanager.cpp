#include "palettemanager.h"
#include <QSettings>

PaletteManager::PaletteManager(const QString &palPath)
    : m_palPath(palPath)
{}

QStringList PaletteManager::paletteNames() const
{
    QSettings s(m_palPath, QSettings::IniFormat);
    // s.setIniCodec("UTF-8"); // Removido para Qt6
    return s.childGroups();
}

void PaletteManager::loadPalette(const QString &name, JingleData jings[JINGLE_COUNT])
{
    QSettings s(m_palPath, QSettings::IniFormat);
    // s.setIniCodec("UTF-8"); // Removido para Qt6
    s.beginGroup(name);

    for (int i = 0; i < JINGLE_COUNT; ++i) {
        jings[i].path   = s.value(QString("Path_%1").arg(i), "").toString();
        jings[i].volume = s.value(QString("Volm_%1").arg(i), 100).toInt();
        jings[i].loop   = s.value(QString("Loop_%1").arg(i), false).toBool();
    }

    s.endGroup();
}

void PaletteManager::savePalette(const QString &name, const JingleData jings[JINGLE_COUNT])
{
    QSettings s(m_palPath, QSettings::IniFormat);
    // s.setIniCodec("UTF-8"); // Removido para Qt6
    s.remove(name);
    s.beginGroup(name);

    for (int i = 0; i < JINGLE_COUNT; ++i) {
        if (!jings[i].path.isEmpty()) {
            s.setValue(QString("Path_%1").arg(i), jings[i].path);
            s.setValue(QString("Volm_%1").arg(i), static_cast<int>(jings[i].volume));
            s.setValue(QString("Loop_%1").arg(i), jings[i].loop);
        }
    }

    s.endGroup();
    s.sync();
}

void PaletteManager::deletePalette(const QString &name)
{
    QSettings s(m_palPath, QSettings::IniFormat);
    // s.setIniCodec("UTF-8"); // Removido para Qt6
    s.remove(name);
    s.sync();
}
