#pragma once

#include ".proto_stubs/responses.pb.h"
#include "matchstate.h"
#include "client_to_server.h"
#include "server_to_client.h"
#include "session.h"

#include <QCoreApplication>
#include <QSharedPointer>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QVector>
#include <QMap>
#include <QSharedPointer>

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
    void sendResponseHandler (const RTS::Response& response_oneof, QSharedPointer<Session> session, uint64_t request_id);

private:
    QSharedPointer<NetworkThread> network_thread;
    QMap<uint32_t, QSharedPointer<RoomThread>> rooms;
    QMap<QByteArray, QByteArray> user_passwords;
    uint64_t next_session_id;
    uint64_t next_response_id;
    QMap<uint64_t, QSharedPointer<Session>> sessions;
    QMap<QByteArray, uint64_t> login_session_ids;

    uint64_t nextSessionId ();
    bool clientMatch (const HCCN::ClientToServer::Message& client_transport_message, const Session& session);
    void sendReply (const HCCN::ClientToServer::Message& client_transport_message,
                    const std::optional<uint64_t>& session_id, const std::optional<uint64_t>& request_id, uint64_t response_id, const std::string& msg);
    void sendReply (const Session& session,
                    const std::optional<uint64_t>& session_id, const std::optional<uint64_t>& request_id, uint64_t response_id, const std::string& msg);
    void sendReplyError (const HCCN::ClientToServer::Message& client_transport_message, const std::string& error_message, RTS::ErrorCode error_code);
    void sendReplySessionExpired (const HCCN::ClientToServer::Message& client_transport_message,
                                  const uint64_t session_id, const std::optional<uint64_t>& request_id, uint64_t response_id);
    void sendReplyRoomList (const Session& session, const uint64_t session_id, const std::optional<uint64_t>& request_id, uint64_t response_id);
    QSharedPointer<Session> validateSessionRequest (const HCCN::ClientToServer::Message& client_transport_message, uint64_t* session_id_ptr);
    void loadRoomList ();
    void storeRoomList ();
};
