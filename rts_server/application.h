#pragma once

#include ".proto_stubs/session_level.pb.h"
#include "matchstate.h"
#include "client_to_server.h"
#include "server_to_client.h"

#include <QCoreApplication>
#include <QSharedPointer>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QVector>
#include <QMap>
#include <QSharedPointer>

// TODO: Locking sessions
struct Session {
    Session () = delete;
    Session (quint64 session_id)
        : session_id (session_id)
    {
    }
    Session (QHostAddress client_address, quint16 client_port, QByteArray login, quint64 session_id)
        : client_address (client_address)
        , client_port (client_port)
        , login (login)
        , session_id (session_id)
    {
    }

    QHostAddress client_address;
    quint16 client_port;
    QByteArray login;
    quint64 session_id;
    std::optional<quint32> current_room = {};
    RTS::Role current_role = RTS::ROLE_UNSPECIFIED;
    std::optional<Unit::Team> current_team = {};
    bool query_room_list_requested = false;
    bool ready = false;
};

class NetworkThread;
class RoomThread;

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
    void sessionTransportClientToServerMessageHandler (const QSharedPointer<HCCN::ClientToServer::Message>& datagram);
    void sendResponseHandler (const RTS::Response& response_oneof, QSharedPointer<Session> session, quint64 request_id);

private:
    QSharedPointer<NetworkThread> network_thread;
    QMap<quint32, QSharedPointer<RoomThread>> rooms;
    QMap<QByteArray, QByteArray> user_passwords;
    quint64 next_session_id;
    quint64 next_response_id;
    QMap<quint64, QSharedPointer<Session>> sessions;
    QMap<QByteArray, quint64> login_session_ids;

    quint64 nextSessionId ();
    bool clientMatch (const HCCN::ClientToServer::Message& client_transport_message, const Session& session);
    void sendReply (const HCCN::ClientToServer::Message& client_transport_message,
                    const std::optional<quint64>& session_id, const std::optional<quint64>& request_id, quint64 response_id, const std::string& msg);
    void sendReply (const Session& session,
                    const std::optional<quint64>& session_id, const std::optional<quint64>& request_id, quint64 response_id, const std::string& msg);
    void sendReplyError (const HCCN::ClientToServer::Message& client_transport_message, const std::string& error_message, RTS::ErrorCode error_code);
    void sendReplySessionExpired (const HCCN::ClientToServer::Message& client_transport_message,
                                  const quint64 session_id, const std::optional<quint64>& request_id, quint64 response_id);
    void sendReplyRoomList (const Session& session, const quint64 session_id, const std::optional<quint64>& request_id, quint64 response_id);
    QSharedPointer<Session> validateSessionRequest (const HCCN::ClientToServer::Message& client_transport_message, quint64* session_id_ptr);
    void loadRoomList ();
    void storeRoomList ();
};
