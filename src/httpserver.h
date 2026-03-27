#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class MainWindow;

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(MainWindow *mainWindow, QObject *parent = nullptr);
    void start(int port);
    void stop();
    int port() const { return m_port; }

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    QTcpServer *m_server = nullptr;
    MainWindow *m_mainWindow = nullptr;
    int m_port = 8000;
};
