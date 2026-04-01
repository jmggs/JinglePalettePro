#pragma once
#include <QString>

class ErrorLogger {
public:
    static ErrorLogger &instance();

    void setEnabled(bool on) { m_enabled = on; }
    void setFilePath(const QString &path) { m_path = path; }

    void log(const QString &code = QString(), const QString &desc = QString());

private:
    ErrorLogger() = default;
    bool    m_enabled = false;
    QString m_path;
};
