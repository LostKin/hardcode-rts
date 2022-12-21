#pragma once

#include <QCoreApplication>
#include <QSharedPointer>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QVector>
#include <QMap>

struct SessionInfo {
    int32_t current_room;
    //std::string login;
};




class NetworkThread;

class Room {
public:
    uint32_t id;
    NetworkThread* network_thread;  

    Room();
};

class Application: public QCoreApplication
{
    Q_OBJECT

public:
    Application (int& argc, char** argv);
    ~Application ();

    bool init ();

private slots:
    void sessionDatagramHandler (QSharedPointer<QNetworkDatagram> datagram);

private:
    NetworkThread* network_thread;
    QMap<QByteArray, QByteArray> users;
    uint64_t token;
    uint64_t MOD;
    QMap<uint64_t, SessionInfo> info;
    QMap<QByteArray, uint64_t> login_tokens;
    Room room;

    uint64_t next_token();
    void sendReply(QSharedPointer<QNetworkDatagram> datagram, const std::string& msg);
};
