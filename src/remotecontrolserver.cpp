#include "remotecontrolserver.h"
#include "globals.h"
#include <QRegularExpression>
#include <QDebug>

RemoteControlServer::RemoteControlServer(QObject *parent)
    : QObject(parent)
{
    m_server = nullptr;
}

RemoteControlServer::~RemoteControlServer() {
    stopServer();
}

void RemoteControlServer::startServer(int port) {
    m_port = port;
    if (m_server) {
        if (m_server->isListening()) {
            qDebug() << "RemoteControlServer: Already listening on port" << m_port;
            return;
        }
        m_server->deleteLater();
        m_server = nullptr;
    }
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &RemoteControlServer::onNewConnection);
    if (!m_server->listen(QHostAddress::Any, m_port)) {
        qWarning() << "RemoteControlServer: Failed to listen on port" << m_port;
    } else {
        qDebug() << "RemoteControlServer: Listening on port" << m_port;
    }
}

void RemoteControlServer::stopServer() {
    if (m_server) {
        qDebug() << "RemoteControlServer: Stopping server on port" << m_port;
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}





void RemoteControlServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        qDebug() << "RemoteControlServer: New connection from" << client->peerAddress().toString();
        connect(client, &QTcpSocket::readyRead, this, &RemoteControlServer::onReadyRead);
        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    }
}

void RemoteControlServer::onReadyRead() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    QByteArray req = client->readAll();
    QString request = QString::fromUtf8(req);
    qDebug() << "RemoteControlServer: Received request:" << request;
    QRegularExpression re("GET /([0-9]{2}|stop|pause|automix|autorepeat) ");
    QRegularExpressionMatch match = re.match(request);
    QString resp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK\n";
    if (match.hasMatch()) {
        QString cmd = match.captured(1);
        qDebug() << "RemoteControlServer: Command received:" << cmd;
        if (cmd.length() == 2 && cmd[0].isDigit() && cmd[1].isDigit()) {
            int idx = cmd.toInt() - 1;
            if (idx >= 0 && idx < JINGLE_COUNT) emit playJingle(idx);
        } else if (cmd == "stop") {
            emit stop();
        } else if (cmd == "pause") {
            emit pause();
        } else if (cmd == "automix") {
            emit autoMix();
        } else if (cmd == "autorepeat") {
            emit autoRepeat();
        }
    } else {
        resp = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nNot found\n";
    }
    client->write(resp.toUtf8());
    client->disconnectFromHost();
}
