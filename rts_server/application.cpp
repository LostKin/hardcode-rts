#include "application.h"
#include "network_thread.h"
#include "room_thread.h"
#include <random>
#include <QFile>


/*SessionInfo::SessionInfo(int32_t id, std::string _login) {
    current_room = id;
    login = _login;
}*/

//Room::Room() {}

Application::Application (int& argc, char** argv)
    : QCoreApplication (argc, argv)
{
    next_session_token = std::mt19937_64 (time (nullptr)) () & 0x7fffffffffffffffULL;
    network_thread.reset (new NetworkThread ("127.0.0.1", 1331, this));
    //room_thread.reset(new RoomThread(this));
    connect (&*network_thread, &NetworkThread::datagramReceived, this, &Application::sessionDatagramHandler);
     //room.network_thread = new NetworkThread ("127.0.0.1", 1332, this);
    //room.id = 0;
}

void Application::receiveResponseHandler(RTS::Response response_oneof, QSharedPointer<Session> session) {
    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (*session, message);
}

quint64 Application::nextSessionToken() {
    return next_session_token++;
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
            qDebug () << "Invalid line" << line << "at" << fname << ":" << "2 fields expected";
            return false;
        }
        user_passwords[fields.at (0)] = fields.at (1);
        qDebug () << fields.at (0) << "->" << fields.at (1);
    }
    //users["login"] = "a";
    //qDebug() << test << "\n";
    //qDebug() << users[test].data();
    network_thread->start ();

    return true;
}
bool Application::clientMatch (const QNetworkDatagram& client_datagram, const Session& session)
{
    return client_datagram.senderAddress () == session.client_address && client_datagram.senderPort () == session.client_port;
}
void Application::sendReply (const QNetworkDatagram& client_datagram, const std::string& message)
{
    network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), client_datagram.senderAddress (), client_datagram.senderPort ()));
}
void Application::sendReply (const Session& session, const std::string& message)
{
    network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), session.client_address, session.client_port));
}
void Application::sendReplyError (const QNetworkDatagram& client_datagram, const std::string& error_message, RTS::ErrorCode error_code)
{
    RTS::Response response_oneof;
    RTS::ErrorResponse* response = response_oneof.mutable_error ();
    setError (response->mutable_error (), error_message, error_code);

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (client_datagram, message);
}
void Application::sendReplySessionExpired (const QNetworkDatagram& client_datagram, quint64 session_token)
{
    RTS::Response response_oneof;
    RTS::SessionClosedResponse* response = response_oneof.mutable_session_closed ();
    response->mutable_session_token ()->set_value (session_token);
    setError (response->mutable_error (), "Session expired", RTS::SESSION_EXPIRED);

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (client_datagram, message);
}
void Application::sendReplyRoomList (const Session& session, quint64 session_token)
{
    RTS::Response response_oneof;
    RTS::RoomListResponse* response = response_oneof.mutable_room_list ();
    response->mutable_session_token ()->set_value (session_token);
    google::protobuf::RepeatedPtrField<RTS::RoomInfo>* room_info_list = response->mutable_room_info_list ();
    for (QMap<quint32, QString>::const_iterator room_it = rooms.constBegin (); room_it != rooms.constEnd (); ++room_it) {
        RTS::RoomInfo* room_info = room_info_list->Add ();
        room_info->set_id (room_it.key ());
        room_info->set_name (room_it.value ().toStdString ());
    }

    std::string message;
    response_oneof.SerializeToString (&message);
    sendReply (session, message);
}
void Application::sessionDatagramHandler (QSharedPointer<QNetworkDatagram> datagram)
{
    QByteArray data = datagram->data();

    RTS::Request request_oneof;
    if (!request_oneof.ParseFromArray (data.data(), data.size())) {
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
            response->mutable_error ()->set_message ("invalid login or password", RTS::LOGIN_FAILED);

            std::string message;
            response_oneof.SerializeToString (&message);
            sendReply (*datagram, message);
            break;
        }

        QMap<QByteArray, quint64>::iterator old_session_token_it = login_session_tokens.find (login);
        if (old_session_token_it != login_session_tokens.end ()) {
            quint64 old_session_token = *old_session_token_it;
            // TODO: Actual cleanup
            sessions.remove (old_session_token);
        }
        quint64 session_token = nextSessionToken ();
        Session session = {
            .client_address = datagram->senderAddress (),
            .client_port = quint16 (datagram->senderPort ()),
            .login = login,
        };
        QSharedPointer<Session> session_ptr = QSharedPointer<Session>(new Session());
        *session_ptr = session;
        sessions[session_token] = session_ptr;
        login_session_tokens[login] = session_token;

        RTS::Response response_oneof;
        RTS::AuthorizationResponse* response = response_oneof.mutable_authorization ();
        response->mutable_session_token ()->set_value (session_token);

        std::string message;
        response_oneof.SerializeToString (&message);
        sendReply (*datagram, message);
    } break;
    case RTS::Request::MessageCase::kQueryRoomList: {
        const RTS::QueryRoomListRequest& request = request_oneof.query_room_list ();
        quint64 session_token;
       QSharedPointer<Session> session = validateSessionRequest (*datagram, request, &session_token);
        if (!session)
            break;
        session->query_room_list_requested = true;

        sendReplyRoomList (*session, session_token);
    } break;
    case RTS::Request::MessageCase::kStopQueryRoomList: {
        const RTS::StopQueryRoomListRequest& request = request_oneof.stop_query_room_list ();
        quint64 session_token;
        QSharedPointer<Session> session = validateSessionRequest (*datagram, request, &session_token);
        if (!session)
            break;
        session->query_room_list_requested = false;
    } break;
    case RTS::Request::MessageCase::kJoinRoom: {
        const RTS::JoinRoomRequest& request = request_oneof.join_room ();
        quint64 session_token;
        QSharedPointer<Session> session = validateSessionRequest (*datagram, request, &session_token);
        if (!session)
            break;
        quint64 request_token;
        if (!validateRequestToken (*datagram, request, &request_token))
            break;
        if (session->current_room.has_value ()) {
            RTS::Response response_oneof;
            RTS::JoinRoomResponse* response = response_oneof.mutable_join_room ();
            setError (response->mutable_error (), "Already joined room", RTS::ALREADY_JOINED_ROOM);

            std::string message;
            response_oneof.SerializeToString (&message);
            sendReply (*session, message);
            break;
        }

        // TODO: Actually verify join room
        session->current_room = request.room_id ();

        RTS::Response response_oneof;
        RTS::JoinRoomResponse* response = response_oneof.mutable_join_room ();
        response->mutable_request_token ()->set_value (request_token);
        response->mutable_success ();

        std::string message;
        response_oneof.SerializeToString (&message);

        sendReply (*session, message);
    } break;
    case RTS::Request::MessageCase::kCreateRoom: {
        const RTS::CreateRoomRequest& request = request_oneof.create_room ();
        quint64 session_token;
        QSharedPointer<Session> session = validateSessionRequest (*datagram, request, &session_token);
        if (!session)
            break;
        quint64 request_token;
        if (!validateRequestToken (*datagram, request, &request_token))
            break;
        quint32 new_room_id = 0;
        if (!rooms.empty ()) {
            QVector<quint32> sorted_room_ids;
            for (QMap<quint32, QString>::const_iterator room_it = rooms.constBegin (); room_it != rooms.constEnd (); ++room_it)
                sorted_room_ids.append (room_it.key ());
            std::sort (sorted_room_ids.begin (), sorted_room_ids.end ());
            for (quint32 room_id: sorted_room_ids) {
                if (new_room_id < room_id)
                    break;
                new_room_id = room_id + 1;
            }
        }
        room_list[new_room_id].reset(new RoomThread(this));
        connect (&*room_list[new_room_id], &RoomThread::receiveRequest, &*room_list[new_room_id], &RoomThread::receiveRequestHandler);
        connect (&*room_list[new_room_id], &RoomThread::sendResponse, this, &Application::receiveResponseHandler);
        rooms[new_room_id] = QString::fromStdString (request.name ());

        RTS::Response response_oneof;
        RTS::CreateRoomResponse* response = response_oneof.mutable_create_room ();
        response->mutable_request_token ()->set_value (request_token);
        response->set_room_id (new_room_id);

        std::string message;
        response_oneof.SerializeToString (&message);

        sendReply (*session, message);

        for (QMap<quint64, QSharedPointer<Session> >::iterator it = sessions.begin (); it != sessions.end (); ++it)
            sendReplyRoomList (*(it.value ()), it.key ());
    } break;
    case RTS::Request::MessageCase::kDeleteRoom: {
        const RTS::DeleteRoomRequest& request = request_oneof.delete_room ();
        quint64 session_token;
        QSharedPointer<Session> session = validateSessionRequest (*datagram, request, &session_token);
        if (session.isNull())
            break;
        quint64 request_token;
        if (!validateRequestToken (*datagram, request, &request_token))
            break;
        QMap<quint32, QString>::iterator room_it = rooms.find (request.room_id ());
        if (room_it == rooms.end ()) {
            RTS::Response response_oneof;
            RTS::DeleteRoomResponse* response = response_oneof.mutable_delete_room ();
            setError (response->mutable_error (), "Room not found", RTS::ROOM_NOT_FOUND);

            std::string message;
            response_oneof.SerializeToString (&message);

            sendReply (*session, message);
            break;
        }

        rooms.erase (room_it);

        RTS::Response response_oneof;
        RTS::DeleteRoomResponse* response = response_oneof.mutable_delete_room ();
        response->mutable_request_token ()->set_value (request_token);
        response->mutable_success ();

        std::string message;
        response_oneof.SerializeToString (&message);

        sendReply (*session, message);
    } break;
    case RTS::Request::MessageCase::kJoinTeam: {
        //qDebug() << "JOIN TEAM";
        const RTS::JoinTeamRequest& request = request_oneof.join_team ();
        quint64 session_token;
        QSharedPointer<Session> session = validateSessionRequest (*datagram, request, &session_token);
        /*
        if (!session)
            break;
        quint64 request_token;
        if (!validateRequestToken (*datagram, request, &request_token))
            break;
        */
        emit room_list[session->current_room.value()]->receiveRequest(request_oneof, session);
    } break;
    case RTS::Request::MessageCase::kReady: {
        const RTS::ReadyRequest& request = request_oneof.ready ();
        quint64 session_token;
        QSharedPointer<Session> session = validateSessionRequest (*datagram, request, &session_token);
        /*
        if (!session)
            break;
        quint64 request_token;
        if (!validateRequestToken (*datagram, request, &request_token))
            break;
        */
        emit room_list[session->current_room.value()]->receiveRequest(request_oneof, session);
    } break;
    }
}
void Application::setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code)
{
    error->set_message (error_message);
    error->set_code (error_code);
}
template <class Request>
QSharedPointer<Session> Application::validateSessionRequest (const QNetworkDatagram& client_datagram, const Request& request, quint64* session_token_ptr)
{
    if (!request.has_session_token ()) {
        sendReplyError (client_datagram, "Missing 'session_token'", RTS::MALFORMED_MESSAGE);
        return nullptr;
    }
    quint64 session_token = request.session_token ().value ();
    QMap<quint64, QSharedPointer<Session> >::iterator session_it = sessions.find (session_token);
    if (session_it == sessions.end ()) {
        sendReplySessionExpired (client_datagram, session_token);
        return nullptr;
    }
    QSharedPointer<Session> session = *session_it;
    if (!clientMatch (client_datagram, *session)) {
        sendReplyError (client_datagram, "Client address changed since authorization", RTS::MISMATCHED_SENDER);
        return nullptr;
    }
    *session_token_ptr = session_token;
    return session;
}
template <class Request>
bool Application::validateRequestToken (const QNetworkDatagram& client_datagram, const Request& request, quint64* request_token_ptr)
{
    if (!request.has_request_token ()) {
        sendReplyError (client_datagram, "Missing 'request_token'", RTS::MALFORMED_MESSAGE);
        return false;
    }
    *request_token_ptr = request.request_token ().value ();
    return true;
}
