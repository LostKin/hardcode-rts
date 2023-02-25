#include "application.h"
#include "authorizationwidget.h"
#include "authorizationprogresswidget.h"
#include "lobbywidget.h"
#include "roomwidget.h"


#include <QDebug>
#include <QFontDatabase>


Application::Application (int& argc, char** argv)
    : QApplication (argc, argv)
{
    QFontDatabase::addApplicationFont (":/fonts/AnkaCoder-bi.ttf");
    QFontDatabase::addApplicationFont (":/fonts/AnkaCoder-b.ttf");
    QFontDatabase::addApplicationFont (":/fonts/AnkaCoder-i.ttf");
    QFontDatabase::addApplicationFont (":/fonts/AnkaCoder-r.ttf");

    QFontDatabase::addApplicationFont (":/fonts/OpenSans-Italic.ttf");
    QFontDatabase::addApplicationFont (":/fonts/OpenSans-Regular.ttf");
    QFontDatabase::addApplicationFont (":/fonts/OpenSans-SemiboldItalic.ttf");
    QFontDatabase::addApplicationFont (":/fonts/OpenSans-Semibold.ttf");

    QFontDatabase::addApplicationFont (":/fonts/NotCourierSans-Bold.otf");
    QFontDatabase::addApplicationFont (":/fonts/NotCourierSans.otf");

    // TODO: Implement proper dialog switches and uncomment: setQuitOnLastWindowClosed (false);
    network_thread.reset (new NetworkThread ("127.0.0.1", 1331, this));
    connect (&*network_thread, &NetworkThread::datagramReceived, this, &Application::sessionDatagramHandler);
}
Application::~Application ()
{
    network_thread->exit ();
    network_thread->wait ();
}

void Application::start ()
{
    network_thread->start ();
    authorizationPromptCallback ();
}

void Application::joinTeam (RTS::Team team) {
    RTS::Request request_oneof;
    RTS::JoinTeamRequest* request = request_oneof.mutable_join_team ();
    request->mutable_session_token ()->set_value (session_token.value ());
    request->mutable_request_token ()->set_value (request_token++);
    request->set_team(team);
    std::string message;
    request_oneof.SerializeToString (&message);
    network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), this->host_address, this->port));
}

void Application::readinessCallback () {
    RTS::Request request_oneof;
    RTS::ReadyRequest* request = request_oneof.mutable_ready ();
    request->mutable_session_token ()->set_value (session_token.value ());
    request->mutable_request_token ()->set_value (request_token++);
    std::string message;
    request_oneof.SerializeToString (&message);
    network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), this->host_address, this->port));
}

void Application::matchStartCallback () {
    //
}

void Application::joinRedTeamCallback () {
    //qDebug() << "callback!";
    joinTeam (RTS::RED);
}

void Application::joinBlueTeamCallback () {
    joinTeam (RTS::BLUE);
}
void Application::joinSpectatorCallback () {
    joinTeam (RTS::SPECTATOR);
}

void Application::quitCallback ()
{
    exit (0);
}
void Application::authorizationPromptCallback ()
{
    AuthorizationWidget* authorization_widget = new AuthorizationWidget;
    connect (authorization_widget, SIGNAL (windowsCloseRequested ()), this, SLOT (quitCallback ()));
    connect (authorization_widget, SIGNAL (loginRequested (const QString&, quint16, const QString&, const QString&)),
             this, SLOT (loginCallback (const QString&, quint16, const QString&, const QString&)));
    current_window.reset (authorization_widget);
    current_window->show ();
}
void Application::loginCallback (const QString& host, quint16 port, const QString& login, const QString& password)
{
    this->host_address = QHostAddress (host);
    this->port = port;
    this->login = login;

    AuthorizationProgressWidget* authorization_progress_widget = new AuthorizationProgressWidget ("Connecting to '" + host + ":" + QString::number (port) + "' as '" + login + "'");
    connect (authorization_progress_widget, SIGNAL (cancelRequested ()), this, SLOT (authorizationPromptCallback ()));
    RTS::Request request_oneof;
    RTS::AuthorizationRequest* request = request_oneof.mutable_authorization ();
    request->set_login (login.toStdString ());
    request->set_password (password.toStdString ());

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), this->host_address, this->port));

    current_window.reset (authorization_progress_widget);
    current_window->show ();
}
void Application::createRoomCallback (const QString& name)
{
    if (!session_token.has_value ())
        return;

    RTS::Request request_oneof;
    RTS::CreateRoomRequest* request = request_oneof.mutable_create_room ();
    request->mutable_session_token ()->set_value (session_token.value ());
    request->mutable_request_token ()->set_value (request_token++);
    request->set_name (name.toStdString ());

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), this->host_address, this->port));
}
void Application::joinRoomCallback (quint32 room_id)
{
    if (!session_token.has_value ())
        return;

    RTS::Request request_oneof;
    RTS::JoinRoomRequest* request = request_oneof.mutable_join_room ();
    request->mutable_session_token ()->set_value (session_token.value ());
    request->mutable_request_token ()->set_value (request_token++);
    request->set_room_id (room_id);

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), this->host_address, this->port));
}
void Application::sessionDatagramHandler (QSharedPointer<QNetworkDatagram> datagram)
{
    QByteArray data = datagram->data ();
    RTS::Response response_oneof;
    if (!response_oneof.ParseFromArray (data.data (), data.size ())) {
        qDebug () << "Invalid response from server: failed to parse protobuf message";
        return;
    }
    switch (response_oneof.message_case ()) {
    case RTS::Response::MessageCase::kAuthorization: {
        const RTS::AuthorizationResponse& response = response_oneof.authorization ();
        switch (response.response_case ()) {
        case RTS::AuthorizationResponse::kError: {
            const RTS::Error& error = response.error ();
            qDebug () << "Response -> AuthorizationResponse -> ERROR:" << QString::fromStdString (error.message ());
        } break;
        case RTS::AuthorizationResponse::kSessionToken: {
            const RTS::SessionToken& session_token = response.session_token ();
            this->session_token = session_token.value ();
            LobbyWidget* lobby_widget = new LobbyWidget (login);
            connect (this, SIGNAL (roomListUpdated (const QVector<RoomEntry>&)), lobby_widget, SLOT (setRoomList (const QVector<RoomEntry>&)));
            connect (lobby_widget, SIGNAL (createRoomRequested (const QString&)), this, SLOT (createRoomCallback (const QString&)));
            connect (lobby_widget, SIGNAL (joinRoomRequested (quint32)), this, SLOT (joinRoomCallback (quint32)));
            current_window.reset (lobby_widget);
            current_window->show ();

            RTS::Request request_oneof;
            RTS::QueryRoomListRequest* request = request_oneof.mutable_query_room_list ();
            request->mutable_session_token ()->set_value (*this->session_token);

            std::string message;
            request_oneof.SerializeToString (&message);

            network_thread->sendDatagram (QNetworkDatagram (QByteArray::fromStdString (message), host_address, port));
        } break;
        default: {
            qDebug () << "Response -> AuthorizationResponse -> UNKNOWN:" << response.response_case ();
        }
        }
    } break;
    case RTS::Response::MessageCase::kJoinRoom: {
        RoomWidget* room_widget = new RoomWidget; // connect roomwidget signals ...
        connect(room_widget, &RoomWidget::joinRedTeamRequested, this, &Application::joinRedTeamCallback);
        connect(room_widget, &RoomWidget::joinBlueTeamRequested, this, &Application::joinBlueTeamCallback);
        connect(room_widget, &RoomWidget::spectateRequested, this, &Application::joinSpectatorCallback);
        connect(this, &Application::queryReadiness, room_widget, &RoomWidget::readinessHandler);
        connect(room_widget, &RoomWidget::readinessRequested, this, &Application::readinessCallback);
        connect(this, &Application::startMatch, room_widget, &RoomWidget::startMatchHandler);
        connect(this, &Application::startCountdown, room_widget, &RoomWidget::startCountDownHandler);
        room_widget->grabMouse ();
        room_widget->grabKeyboard ();
        room_widget->showFullScreen ();
        current_window.reset (room_widget);
    } break;
    case RTS::Response::MessageCase::kCreateRoom: {
    } break;
    case RTS::Response::MessageCase::kDeleteRoom: {
    } break;
    case RTS::Response::MessageCase::kRoomList: {
        const RTS::RoomListResponse& response = response_oneof.room_list ();
        const google::protobuf::RepeatedPtrField<RTS::RoomInfo>& room_info_list = response.room_info_list ();
        QVector<RoomEntry> room_list;
        for (google::protobuf::RepeatedPtrField<RTS::RoomInfo>::const_iterator it = room_info_list.cbegin (); it != room_info_list.cend (); ++it) {
            const RTS::RoomInfo& room_info = *it;
            room_list.append (RoomEntry (room_info.id (), QString::fromStdString (room_info.name ())));
        }
        emit roomListUpdated (room_list);
    } break;
    case RTS::Response::MessageCase::kJoinTeam: {
        // lets manage some shit
        emit queryReadiness();
    } break;
    case RTS::Response::MessageCase::kReady: {
        emit startCountdown ();
    } break;
    case RTS::Response::MessageCase::kMatchStart: {
        qDebug() << "Ready respose caught";
        emit startMatch ();
    } break;
    default: {
        qDebug () << "Response -> UNKNOWN:" << response_oneof.message_case ();
    }
    }
}
