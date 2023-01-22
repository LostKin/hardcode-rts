#pragma once

#include "network_thread.h"
#include "roomentry.h"

#include <QApplication>
#include <QSharedPointer>

class QNetworkDatagram;

class Application: public QApplication
{
    Q_OBJECT

public:
    Application (int& argc, char** argv);
    ~Application ();
    void start ();

signals:
    void roomListUpdated (const QVector<RoomEntry>& room_list);

private:
    void showLobby ();

private slots:
    void quitCallback ();
    void authorizationPromptCallback ();
    void loginCallback (const QString& host, quint16 port, const QString& login, const QString& password);
    void createRoomCallback (const QString& name);
    void joinRoomCallback (quint32 name);
    void sessionDatagramHandler (QSharedPointer<QNetworkDatagram> datagram);

private:
    QSharedPointer<QWidget> current_window;
    QSharedPointer<NetworkThread> network_thread;
    QHostAddress host_address;
    quint16 port;
    QString login;
    std::optional<quint64> session_token;
    quint64 request_token = 0;
};
