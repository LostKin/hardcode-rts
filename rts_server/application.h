#pragma once

#include ".proto_stubs/session_level.pb.h"
#include "matchstate.h"

#include <QCoreApplication>
#include <QSharedPointer>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QVector>
#include <QMap>
#include <QSharedPointer>

struct Session {
    QHostAddress client_address;
    quint16 client_port;
    std::optional<quint32> current_room = {};
    RTS::Role current_role = RTS::ROLE_UNSPECIFIED;
    std::optional<Unit::Team> current_team = {};
    QByteArray login;
    bool query_room_list_requested = false;
    bool ready = false;
};

class NetworkThread;
class RoomThread;

/*class Room {
public:
    quint32 id;
    NetworkThread* network_thread;

    Room();
};*/

class Application: public QCoreApplication
{
    Q_OBJECT

public:
    Application (int& argc, char** argv);
    ~Application ();

    bool init ();

private:
    void setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code);

private slots:
    void sessionDatagramHandler (QSharedPointer<QNetworkDatagram> datagram);
    void receiveResponseHandler (RTS::Response response_oneof, QSharedPointer<Session> session);

private:
    QSharedPointer<NetworkThread> network_thread;
    QMap<quint32, QSharedPointer<RoomThread>> room_list;
    QMap<QByteArray, QByteArray> user_passwords;
    quint64 next_session_token;
    QMap<quint64, QSharedPointer<Session>> sessions;
    QMap<QByteArray, quint64> login_session_tokens;
    // Room room;
    QMap<quint32, QString> rooms;

    quint64 nextSessionToken ();
    bool clientMatch (const QNetworkDatagram& datagram, const Session& session);
    void sendReply (const QNetworkDatagram& client_datagram, const std::string& msg);
    void sendReply (const Session& session, const std::string& msg);
    void sendReplyError (const QNetworkDatagram& client_datagram, const std::string& error_message, RTS::ErrorCode error_code);
    void sendReplySessionExpired (const QNetworkDatagram& client_datagram, quint64 session_token);
    void sendReplyRoomList (const Session& session, quint64 session_token);
    template <class Request>
    QSharedPointer<Session> validateSessionRequest (const QNetworkDatagram& client_datagram, const Request& request, quint64* session_token_ptr);
    template <class Request>
    bool validateRequestToken (const QNetworkDatagram& client_datagram, const Request& request, quint64* request_token_ptr);
    void loadRoomList ();
    void storeRoomList ();
};
