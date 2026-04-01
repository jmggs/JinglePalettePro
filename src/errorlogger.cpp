#include "errorlogger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

ErrorLogger &ErrorLogger::instance()
{
    static ErrorLogger inst;
    return inst;
}

void ErrorLogger::log(const QString &code, const QString &desc)
{
    if (!m_enabled || m_path.isEmpty()) return;
    QFile f(m_path);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            << ", Code " << code << ": " << desc << "\n";
    }
}
