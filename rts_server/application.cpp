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
    // room_thread.reset(new RoomThread(this));
    connect (&*network_thread, &NetworkThread::datagramReceived, this, &Application::sessionTransportClientToServerMessageHandler);
    // room.network_thread = new NetworkThread ("127.0.0.1", 1332, this);
    // room.id = 0;
}

void Application::sendResponseHandler (const RTS::Response& response_oneof, QSharedPointer<Session> session, quint64 request_id)
{
    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (*session, session->session_id, request_id, next_response_id++, message);
}

quint64 Application::nextSessionId ()
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
        user_passwords[fields.at (0)] = fields.at (1);
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
                             const std::optional<quint64>& session_id, const std::optional<quint64>& request_id, quint64 response_id, const std::string& message)
{
    QSharedPointer<HCCN::ServerToClient::Message> m (new HCCN::ServerToClient::Message (client_transport_message.host, client_transport_message.port,
                                                                                        session_id, request_id, response_id, {message.data (), qsizetype (message.size ())}));
    network_thread->sendDatagram (m);
}
void Application::sendReply (const Session& session,
                             const std::optional<quint64>& session_id, const std::optional<quint64>& request_id, quint64 response_id, const std::string& message)
{
    QSharedPointer<HCCN::ServerToClient::Message> datagram (new HCCN::ServerToClient::Message (session.client_address, session.client_port,
                                                                                               session_id, request_id, response_id, {message.data (), qsizetype (message.size ())}));
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
                                           quint64 session_id, const std::optional<quint64>& request_id, quint64 response_id)
{
    RTS::Response response_oneof;
    RTS::SessionClosedResponse* response = response_oneof.mutable_session_closed ();
    setError (response->mutable_error (), "Session expired", RTS::ERROR_CODE_SESSION_EXPIRED);

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (client_transport_message, session_id, request_id, response_id, message);
}
void Application::sendReplyRoomList (const Session& session,
                                     const quint64 session_id, const std::optional<quint64>& request_id, quint64 response_id)
{
    RTS::Response response_oneof;
    RTS::RoomListResponse* response = response_oneof.mutable_room_list ();
    google::protobuf::RepeatedPtrField<RTS::RoomInfo>* room_info_list = response->mutable_room_info_list ();
    QMap<quint32, quint32> room_client_counters;
    for (QMap<quint64, QSharedPointer<Session>>::const_iterator it = sessions.constBegin (); it != sessions.constEnd (); ++it) {
        if ((*it)->current_room.has_value ())
            room_client_counters[(*it)->current_room.value ()]++;
    }
    for (QMap<quint32, QSharedPointer<RoomThread>>::const_iterator room_it = rooms.constBegin (); room_it != rooms.constEnd (); ++room_it) {
        const RoomThread& room_thread = **room_it;
        RTS::RoomInfo* room_info = room_info_list->Add ();
        room_info->set_id (room_it.key ());
        room_info->set_name (room_thread.name ().toStdString ());
        room_info->set_client_count (room_client_counters[room_it.key ()]);
        room_info->set_player_count (room_thread.playerCount ());
        room_info->set_ready_player_count (room_thread.readyPlayerCount ());
        room_info->set_spectator_count (room_thread.spectatorCount ());
    }

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (session, session_id, request_id, response_id, message);
}
void Application::sessionTransportClientToServerMessageHandler (const QSharedPointer<HCCN::ClientToServer::Message>& transport_message)
{
    const QByteArray& data = transport_message->message;

    RTS::Request request_oneof;
    if (!request_oneof.ParseFromArray (data.data (), data.size ())) {
        qDebug () << "Failed to parse message from client";
        return;
    }

    switch (request_oneof.message_case ()) {
    case RTS::Request::MessageCase::kAuthorization: {
        const RTS::AuthorizationRequest& request = request_oneof.authorization ();
        QByteArray login = QByteArray::fromStdString (request.login ());
        QByteArray password = QByteArray::fromStdString (request.password ());
        QMap<QByteArray, QByteArray>::iterator password_it = user_passwords.find (login);
        if (password_it == user_passwords.end () || password != *password_it) {
            RTS::Response response_oneof;
            RTS::AuthorizationResponse* response = response_oneof.mutable_authorization ();
            response->mutable_error ()->set_message ("invalid login or password", RTS::ERROR_CODE_LOGIN_FAILED);

            std::string message;
            response_oneof.SerializeToString (&message);
            sendReply (*transport_message, {}, transport_message->request_id, next_response_id++, message);
            break;
        }

        QMap<QByteArray, quint64>::iterator old_session_id_it = login_session_ids.find (login);
        if (old_session_id_it != login_session_ids.end ()) {
            quint64 old_session_id = *old_session_id_it;
            // TODO: Actual cleanup
            sessions.remove (old_session_id);
        }
        quint64 session_id = nextSessionId ();
        QSharedPointer<Session> session = QSharedPointer<Session> (new Session (transport_message->host, transport_message->port, login, session_id));
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
        quint64 session_id;
        QSharedPointer<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        session->query_room_list_requested = true;

        sendReplyRoomList (*session, session_id, transport_message->request_id, next_response_id++);
    } break;
    case RTS::Request::MessageCase::kStopQueryRoomList: {
        quint64 session_id;
        QSharedPointer<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        session->query_room_list_requested = false;
    } break;
    case RTS::Request::MessageCase::kJoinRoom: {
        const RTS::JoinRoomRequest& request = request_oneof.join_room ();
        quint64 session_id;
        QSharedPointer<Session> session = validateSessionRequest (*transport_message, &session_id);
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
        quint64 session_id;
        QSharedPointer<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (!session)
            break;
        quint32 new_room_id = 0;
        if (!rooms.empty ()) {
            for (QMap<quint32, QSharedPointer<RoomThread>>::const_iterator room_it = rooms.constBegin (); room_it != rooms.constEnd (); ++room_it)
                new_room_id = qMax (new_room_id, room_it.key ());
            ++new_room_id;
        }
        rooms[new_room_id].reset (new RoomThread (QString::fromStdString (request.name ()), this));
        connect (&*rooms[new_room_id], &RoomThread::receiveRequest, &*rooms[new_room_id], &RoomThread::receiveRequestHandler);
        connect (&*rooms[new_room_id], &RoomThread::sendResponse, this, &Application::sendResponseHandler);

        RTS::Response response_oneof;
        RTS::CreateRoomResponse* response = response_oneof.mutable_create_room ();
        response->set_room_id (new_room_id);

        storeRoomList ();

        std::string message;
        response_oneof.SerializeToString (&message);

        sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);

        for (QMap<quint64, QSharedPointer<Session>>::iterator it = sessions.begin (); it != sessions.end (); ++it)
            sendReplyRoomList (*(it.value ()), it.key (), transport_message->request_id, next_response_id++);
    } break;
    case RTS::Request::MessageCase::kDeleteRoom: {
        const RTS::DeleteRoomRequest& request = request_oneof.delete_room ();
        quint64 session_id;
        QSharedPointer<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (session.isNull ())
            break;
        QMap<quint32, QSharedPointer<RoomThread>>::iterator room_it = rooms.find (request.room_id ());
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
        quint64 session_id;
        QSharedPointer<Session> session = validateSessionRequest (*transport_message, &session_id);
        if (session.isNull ())
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
        QMap<quint32, QSharedPointer<RoomThread>>::iterator it = rooms.find (session->current_room.value ());
        if (it == rooms.end ()) {
            RTS::Response response_oneof;
            RTS::ErrorResponse* response = response_oneof.mutable_error ();
            setError (response->mutable_error (), "Room not found", RTS::ERROR_CODE_ROOM_NOT_FOUND);

            std::string message;
            response_oneof.SerializeToString (&message);

            sendReply (*session, session_id, transport_message->request_id, next_response_id++, message);
            break;
        }

        emit (*it)->receiveRequest (request_oneof, session, transport_message->request_id);
    }
    }
}
void Application::setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code)
{
    error->set_message (error_message);
    error->set_code (error_code);
}
QSharedPointer<Session> Application::validateSessionRequest (const HCCN::ClientToServer::Message& client_transport_message, quint64* session_id_ptr)
{
    if (!client_transport_message.session_id.has_value ()) {
        sendReplyError (client_transport_message, "Missing session id", RTS::ERROR_CODE_MALFORMED_MESSAGE);
        return nullptr;
    }
    quint64 session_id = *client_transport_message.session_id;
    QMap<quint64, QSharedPointer<Session>>::iterator session_it = sessions.find (session_id);
    if (session_it == sessions.end ()) {
        sendReplySessionExpired (client_transport_message, session_id, client_transport_message.request_id, next_response_id++);
        return nullptr;
    }
    QSharedPointer<Session> session = *session_it;
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
        rooms[new_room_id].reset (new RoomThread (room_settings.value ("name").toString (), this));
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
    for (const QSharedPointer<RoomThread>& room_thread: rooms) {
        room_settings.setArrayIndex (i++);
        room_settings.setValue ("name", room_thread->name ());
    }
    room_settings.endArray ();
}
