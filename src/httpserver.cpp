#include "httpserver.h"
#include "mainwindow.h"
#include <QRegularExpression>

HttpServer::HttpServer(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

void HttpServer::start(int port)
{
    m_port = port;
    if (m_server->isListening()) m_server->close();
    m_server->listen(QHostAddress::Any, port);
}

void HttpServer::stop()
{
    if (m_server->isListening()) m_server->close();
}

void HttpServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        connect(client, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
    }
}

void HttpServer::onReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    QByteArray req = client->readAll();
    // Parse GET /XX
    QRegularExpression re("GET \/(\\d{2}) ");
    QRegularExpressionMatch match = re.match(QString::fromUtf8(req));
    if (match.hasMatch()) {
        int idx = match.captured(1).toInt() - 1;
        if (idx >= 0 && idx < 30 && m_mainWindow) {
            QMetaObject::invokeMethod(m_mainWindow, "remoteTriggerJingle", Qt::QueuedConnection, Q_ARG(int, idx));
        }
    }
    // Simple HTTP 200 response
    QByteArray resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\nOK";
    client->write(resp);
    client->disconnectFromHost();
}
