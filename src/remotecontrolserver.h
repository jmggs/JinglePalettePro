#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

class RemoteControlServer : public QObject {
    Q_OBJECT
public:
    explicit RemoteControlServer(QObject *parent = nullptr);
    ~RemoteControlServer();

    void startServer(int port);
    void stopServer();
    bool isRunning() const { return m_server && m_server->isListening(); }
    int  port() const { return m_port; }

signals:
    void playJingle(int idx); // idx: 0..29
    void stop();
    void pause();
    void autoMix();
    void autoRepeat();



private slots:
    void onNewConnection();
    void onReadyRead();

private:
    QTcpServer *m_server = nullptr;
    int m_port = 8000;
    bool m_running = false;
};
