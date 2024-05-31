#pragma once

#include "responses.pb.h"
#include "matchstate.h"
#include "client_to_server.h"
#include "server_to_client.h"
#include "session.h"

#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QVector>
#include <map>
#include <memory>


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
    void sessionTransportClientToServerMessageHandler (const std::shared_ptr<HCCN::ClientToServer::Message>& datagram);
    void sendResponseHandler (const RTS::Response& response_oneof, std::shared_ptr<Session> session, uint64_t request_id);

private:
    std::shared_ptr<NetworkThread> network_thread;
    std::map<uint32_t, std::shared_ptr<RoomThread>> rooms;
    std::map<std::string, std::string> user_passwords;
    uint64_t next_session_id;
    uint64_t next_response_id;
    std::map<uint64_t, std::shared_ptr<Session>> sessions;
    std::map<std::string, uint64_t> login_session_ids;

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
    std::shared_ptr<Session> validateSessionRequest (const HCCN::ClientToServer::Message& client_transport_message, uint64_t* session_id_ptr);
    void loadRoomList ();
    void storeRoomList ();
};
