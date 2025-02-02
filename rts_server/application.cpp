#include "application.h"
#include "network_thread.h"
#include "room_thread.h"

#include <random>
#include <QFile>
#include <QSettings>

/*SessionInfo::SessionInfo(int32_t id, std::string _login) {
    current_room = id;
    login = _login;
}*/

// Room::Room() {}

Application::Application (int& argc, char** argv)
    : QCoreApplication (argc, argv)
{
    next_session_id = std::mt19937_64 (time (nullptr)) () & 0x7fffffffffffffffULL;
    next_response_id = 0;
    network_thread.reset (new NetworkThread ("0.0.0.0", 1331, this));
    connect (&*network_thread, &NetworkThread::datagramReceived, this, &Application::sessionTransportClientToServerMessageHandler);
}

void Application::sendResponseHandler (const RTS::Response& response_oneof, std::shared_ptr<Session> session, uint64_t request_id)
{
    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (*session, session->session_id, request_id, next_response_id++, message);
}

uint64_t Application::nextSessionId ()
{
    return next_session_id++;
}

Application::~Application ()
{
    // TODO: Implement handling SIGINT
    network_thread->exit ();
    network_thread->wait ();
}

bool Application::init ()
{
    const QString fname ("users.txt");
    QFile file (fname);
    if (!file.open (QIODevice::ReadOnly)) {
        qDebug () << "Failed to open file" << fname << ":" << file.errorString ();
        return false;
    }
    while (!file.atEnd ()) {
        QByteArray line = file.readLine ();
        if (!line.size ()) {
            if (file.atEnd ())
                break;
            qDebug () << "Failed to read from file" << fname << ":" << file.errorString ();
            return false;
        }
        if (line[line.size () - 1] == '\n')
            line.resize (line.size () - 1);
        QList<QByteArray> fields = line.split (':');
        if (fields.size () != 2) {
            qDebug () << "Invalid line" << line << "at" << fname << ":"
                      << "2 fields expected";
            return false;
        }
        user_passwords[std::string (fields.at (0).data (), fields.at (0).size ())] = std::string (fields.at (1).data (), fields.at (1).size ());
    }

    loadRoomList ();

    network_thread->start ();

    return true;
}
bool Application::clientMatch (const HCCN::ClientToServer::Message& client_transport_message, const Session& session)
{
    return client_transport_message.host == session.client_address && client_transport_message.port == session.client_port;
}
void Application::sendReply (const HCCN::ClientToServer::Message& client_transport_message,
                             const std::optional<uint64_t>& session_id, const std::optional<uint64_t>& request_id, uint64_t response_id, const std::string& message)
{
    std::shared_ptr<HCCN::ServerToClient::Message> m (new HCCN::ServerToClient::Message (client_transport_message.host, client_transport_message.port,
                                                                                         session_id, request_id, response_id, {message.data (), message.data () + message.size ()}));
    network_thread->sendDatagram (m);
}
void Application::sendReply (const Session& session,
                             const std::optional<uint64_t>& session_id, const std::optional<uint64_t>& request_id, uint64_t response_id, const std::string& message)
{
    std::shared_ptr<HCCN::ServerToClient::Message> datagram (new HCCN::ServerToClient::Message (session.client_address, session.client_port,
                                                                                                session_id, request_id, response_id, {message.data (), message.data () + message.size ()}));
    network_thread->sendDatagram (datagram);
}
void Application::sendReplyError (const HCCN::ClientToServer::Message& client_transport_message, const std::string& error_message, RTS::ErrorCode error_code)
{
    RTS::Response response_oneof;
    RTS::ErrorResponse* response = response_oneof.mutable_error ();
    setError (response->mutable_error (), error_message, error_code);

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (client_transport_message, {}, {}, next_response_id++, message);
}
void Application::sendReplySessionExpired (const HCCN::ClientToServer::Message& client_transport_message,
                                           uint64_t session_id, const std::optional<uint64_t>& request_id, uint64_t response_id)
{
    RTS::Response response_oneof;
    RTS::SessionClosedResponse* response = response_oneof.mutable_session_closed ();
    setError (response->mutable_error (), "Session expired", RTS::ERROR_CODE_SESSION_EXPIRED);

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (client_transport_message, session_id, request_id, response_id, message);
}
void Application::sendReplyRoomList (const Session& session,
                                     const uint64_t session_id, const std::optional<uint64_t>& request_id, uint64_t response_id)
{
    RTS::Response response_oneof;
    RTS::RoomListResponse* response = response_oneof.mutable_room_list ();
    google::protobuf::RepeatedPtrField<RTS::RoomInfo>* room_info_list = response->mutable_room_info_list ();
    std::map<uint32_t, uint32_t> room_client_counters;
    for (std::map<uint64_t, std::shared_ptr<Session>>::const_iterator it = sessions.cbegin (); it != sessions.cend (); ++it) {
        if (it->second->current_room.has_value ())
            room_client_counters[it->second->current_room.value ()]++;
    }
    for (std::map<uint32_t, std::shared_ptr<RoomThread>>::const_iterator room_it = rooms.cbegin (); room_it != rooms.cend (); ++room_it) {
        uint32_t room_id = room_it->first;
        const RoomThread& room_thread = *room_it->second;
        RTS::RoomInfo* room_info = room_info_list->Add ();
        room_info->set_id (room_id);
        room_info->set_name (room_thread.name ());
        room_info->set_client_count (room_client_counters[room_id]);
        room_info->set_player_count (room_thread.playerCount ());
        room_info->set_ready_player_count (room_thread.readyPlayerCount ());
        room_info->set_spectator_count (room_thread.spectatorCount ());
    }

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (session, session_id, request_id, response_id, message);
}
void Application::sessionTransportClientToServerMessageHandler (const std::shared_ptr<HCCN::ClientToServer::Message>& transport_message)
{
    RTS::Request request_oneof;
    if (!request_oneof.ParseFromArray (transport_message->message.data (), transport_message->message.size ())) {
        qDebug () << "Failed to parse message from client";
        return;
    }

    switch (request_oneof.message_case ()) {
    case RTS::Request::MessageCase::kAuthorization: {
        const RTS::AuthorizationRequest& request = request_oneof.authorization ();
        std::string login = request.login ();
        std::string password = request.password ();
        std::map<std::string, std::string>::iterator password_it = user_passwords.find (login);
        if (password_it == user_passwords.end () || password != password_it->second) {
            RTS::Response response_oneof;
            RTS::AuthorizationResponse* response = response_oneof.mutable_authorization ();
            response->mutable_error ()->set_message ("invalid login or password", RTS::ERROR_CODE_LOGIN_FAILED);

            std::string message;
            response_oneof.SerializeToString (&message);
            sendReply (*transport_message, {}, transport_message->request_id, next_response_id++, message);
            break;
        }

        std::map<std::string, uint64_t>::iterator old_session_id_it = login_session_ids.find (login);
        if (old_session_id_it != login_session_ids.end ()) {
            uint64_t old_session_id = old_session_id_it->second;
            // TODO: Actual cleanup
            sessions.erase (old_session_id);
        }
        uint64_t session_id = nextSessionId ();
        std::shared_ptr<Session> session = std::shared_ptr<Session> (new Session (transport_message->host, transport_message->port, login, session_id));
        sessions[session_id] = session;
        login_session_ids[login] = session_id;

        RTS::Response response_oneof;
        RTS::AuthorizationResponse* response = response_oneof.mutable_authorization ();
        response->mutable_session_token ()->set_value (session_id);

        std::string message;
        response_oneof.SerializeToString (&message);
        sendReply (*transport_message, {}, transport_message->request_id, next_response_id++, message);
    } break;
    case RTS::Request::MessageCase::kQueryRoomList: {
        uint64_t session_id;
        std::shared_ptr<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        session->query_room_list_requested = true;

        sendReplyRoomList (*session, session_id, transport_message->request_id, next_response_id++);
    } break;
    case RTS::Request::MessageCase::kStopQueryRoomList: {
        uint64_t session_id;
        std::shared_ptr<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        session->query_room_list_requested = false;
    } break;
    case RTS::Request::MessageCase::kJoinRoom: {
        const RTS::JoinRoomRequest& request = request_oneof.join_room ();
        uint64_t session_id;
        std::shared_ptr<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        if (session->current_room.has_value ()) {
            RTS::Response response_oneof;
            RTS::JoinRoomResponse* response = response_oneof.mutable_join_room ();
            setError (response->mutable_error (), "Already joined room", RTS::ERROR_CODE_ALREADY_JOINED_ROOM);

            std::string message;
            response_oneof.SerializeToString (&message);
            sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);
            break;
        }

        // TODO: Actually verify join room
        session->current_room = request.room_id ();

        RTS::Response response_oneof;
        RTS::JoinRoomResponse* response = response_oneof.mutable_join_room ();
        response->mutable_success ();

        std::string message;
        response_oneof.SerializeToString (&message);

        sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);
    } break;
    case RTS::Request::MessageCase::kCreateRoom: {
        const RTS::CreateRoomRequest& request = request_oneof.create_room ();
        uint64_t session_id;
        std::shared_ptr<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        uint32_t new_room_id = 0;
        if (!rooms.empty ()) {
            for (std::map<uint32_t, std::shared_ptr<RoomThread>>::const_iterator room_it = rooms.cbegin (); room_it != rooms.cend (); ++room_it)
                new_room_id = qMax (new_room_id, room_it->first);
            ++new_room_id;
        }
        rooms[new_room_id].reset (new RoomThread (request.name (), this));
        connect (&*rooms[new_room_id], &RoomThread::receiveRequest, &*rooms[new_room_id], &RoomThread::receiveRequestHandler);
        connect (&*rooms[new_room_id], &RoomThread::sendResponse, this, &Application::sendResponseHandler);

        RTS::Response response_oneof;
        RTS::CreateRoomResponse* response = response_oneof.mutable_create_room ();
        response->set_room_id (new_room_id);

        storeRoomList ();

        std::string message;
        response_oneof.SerializeToString (&message);

        sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);

        for (std::map<uint64_t, std::shared_ptr<Session>>::iterator it = sessions.begin (); it != sessions.end (); ++it)
            sendReplyRoomList (*it->second, it->first, transport_message->request_id, next_response_id++);
    } break;
    case RTS::Request::MessageCase::kDeleteRoom: {
        const RTS::DeleteRoomRequest& request = request_oneof.delete_room ();
        uint64_t session_id;
        std::shared_ptr<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        std::map<uint32_t, std::shared_ptr<RoomThread>>::iterator room_it = rooms.find (request.room_id ());
        if (room_it == rooms.end ()) {
            RTS::Response response_oneof;
            RTS::DeleteRoomResponse* response = response_oneof.mutable_delete_room ();
            setError (response->mutable_error (), "Room not found", RTS::ERROR_CODE_ROOM_NOT_FOUND);

            std::string message;
            response_oneof.SerializeToString (&message);

            sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);
            break;
        }

        rooms.erase (room_it);

        RTS::Response response_oneof;
        RTS::DeleteRoomResponse* response = response_oneof.mutable_delete_room ();
        response->mutable_success ();

        std::string message;
        response_oneof.SerializeToString (&message);

        sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);
    } break;
    default: {
        uint64_t session_id;
        std::shared_ptr<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        if (!session->current_room.has_value ()) {
            RTS::Response response_oneof;
            RTS::ErrorResponse* response = response_oneof.mutable_error ();
            setError (response->mutable_error (), "Not joined room", RTS::ERROR_CODE_NOT_JOINED_ROOM);

            std::string message;
            response_oneof.SerializeToString (&message);

            sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);
            break;
        }
        std::map<uint32_t, std::shared_ptr<RoomThread>>::iterator it = rooms.find (session->current_room.value ());
        if (it == rooms.end ()) {
            RTS::Response response_oneof;
            RTS::ErrorResponse* response = response_oneof.mutable_error ();
            setError (response->mutable_error (), "Room not found", RTS::ERROR_CODE_ROOM_NOT_FOUND);

            std::string message;
            response_oneof.SerializeToString (&message);

            sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);
            break;
        }

        emit (it->second)->receiveRequest (request_oneof, session, transport_message->request_id);
    }
    }
}
void Application::setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code)
{
    error->set_message (error_message);
    error->set_code (error_code);
}
std::shared_ptr<Session> Application::validateSessionRequest (const HCCN::ClientToServer::Message& client_transport_message, uint64_t* session_id_ptr)
{
    if (!client_transport_message.session_id.has_value ()) {
        sendReplyError (client_transport_message, "Missing session id", RTS::ERROR_CODE_MALFORMED_MESSAGE);
        return nullptr;
    }
    uint64_t session_id = *client_transport_message.session_id;
    std::map<uint64_t, std::shared_ptr<Session>>::iterator session_it = sessions.find (session_id);
    if (session_it == sessions.end ()) {
        sendReplySessionExpired (client_transport_message, session_id, client_transport_message.request_id, next_response_id++);
        return nullptr;
    }
    std::shared_ptr<Session> session = session_it->second;
    if (!clientMatch (client_transport_message, *session)) {
        sendReplyError (client_transport_message, "Client address changed since authorization", RTS::ERROR_CODE_MISMATCHED_SENDER);
        return nullptr;
    }
    *session_id_ptr = session_id;
    return session;
}
void Application::loadRoomList ()
{
    QSettings room_settings ("room.ini", QSettings::IniFormat);
    int size = room_settings.beginReadArray ("rooms");
    for (int new_room_id = 0; new_room_id < size; ++new_room_id) {
        room_settings.setArrayIndex (new_room_id);
        rooms[new_room_id].reset (new RoomThread (room_settings.value ("name").toString ().toStdString (), this));
        connect (&*rooms[new_room_id], &RoomThread::receiveRequest, &*rooms[new_room_id], &RoomThread::receiveRequestHandler);
        connect (&*rooms[new_room_id], &RoomThread::sendResponse, this, &Application::sendResponseHandler);
    }
    room_settings.endArray ();
}
void Application::storeRoomList ()
{
    QSettings room_settings ("room.ini", QSettings::IniFormat);
    room_settings.beginWriteArray ("rooms");
    int i = 0;
    for (std::map<uint32_t, std::shared_ptr<RoomThread>>::const_iterator it = rooms.begin (); it != rooms.end (); ++it) {
        RoomThread& room_thread = *it->second;
        room_settings.setArrayIndex (i++);
        room_settings.setValue ("name", QString::fromStdString (room_thread.name ()));
    }
    room_settings.endArray ();
}
