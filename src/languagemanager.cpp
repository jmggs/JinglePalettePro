#include "languagemanager.h"
#include <QSettings>

LanguageManager::LanguageManager(const QString &langPath)
    : m_langPath(langPath), m_lang("English")
{}

void LanguageManager::setLanguage(const QString &lang)
{
    m_lang = lang;
    loadSection(lang);
}

void LanguageManager::loadSection(const QString &lang)
{
    m_strings.clear();
    QSettings s(m_langPath, QSettings::IniFormat);
    // s.setIniCodec("UTF-8"); // Removido para Qt6
    s.beginGroup(lang);
    const auto keys = s.childKeys();
    for (const QString &k : keys) {
        m_strings[k] = s.value(k).toString();
    }
    s.endGroup();

    // Also load English as fallback base
    if (lang != "English") {
        s.beginGroup("English");
        const auto enKeys = s.childKeys();
        for (const QString &k : enKeys) {
            if (!m_strings.contains(k))
                m_strings[k] = s.value(k).toString();
        }
        s.endGroup();
    }
}

QString LanguageManager::entry(const QString &key, const QString &fallback) const
{
    return m_strings.value(key, fallback.isEmpty() ? key : fallback);
}

QStringList LanguageManager::availableLanguages() const
{
    QSettings s(m_langPath, QSettings::IniFormat);
    // s.setIniCodec("UTF-8"); // Removido para Qt6
    return s.childGroups();
}
