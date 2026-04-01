#pragma once
#include <QString>
#include <QHash>

class LanguageManager {
public:
    explicit LanguageManager(const QString &langPath);

    void setLanguage(const QString &lang);
    QString language() const { return m_lang; }

    // Returns the translated string for key, or fallback if not found
    QString entry(const QString &key, const QString &fallback = QString()) const;

    // Returns all available language names
    QStringList availableLanguages() const;

private:
    void loadSection(const QString &lang);

    QString                  m_langPath;
    QString                  m_lang;
    QHash<QString, QString>  m_strings;
};
