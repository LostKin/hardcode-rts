#include "application.h"
#include "authorizationwidget.h"
#include "authorizationprogresswidget.h"
#include "lobbywidget.h"
#include "matchstate.h"
#include "requests.pb.h"

#include <QDebug>
#include <QDateTime>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSettings>

Application::Application (int& argc, char** argv)
    : QApplication (argc, argv)
{
    if (argc == 2 && !strcmp (argv[1], "--single-mode"))
        single_mode = true;

    setAttribute (Qt::AA_CompressHighFrequencyEvents, false);

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
    network_thread.reset (new NetworkThread (this));
    connect (&*network_thread, &NetworkThread::datagramReceived, this, &Application::sessionDatagramHandler);
}
Application::~Application ()
{
    if (current_window)
        current_window->deleteLater ();

    network_thread->exit ();
    network_thread->wait ();
}

void Application::start ()
{
    network_thread->start ();
    if (single_mode)
        showRoom (true);
    else
        authorizationPromptCallback ();
}
void Application::selectRolePlayer ()
{
    RTS::Request request_oneof;
    RTS::SelectRoleRequest* request = request_oneof.mutable_select_role ();
    request->set_role (RTS::Role::ROLE_PLAYER);
    std::string message;
    request_oneof.SerializeToString (&message);
    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, QByteArray::fromStdString (message)});
}
void Application::readinessCallback ()
{
    RTS::Request request_oneof;
    RTS::ReadyRequest* request = request_oneof.mutable_ready ();
    (void) request;
    std::string message;
    request_oneof.SerializeToString (&message);
    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, QByteArray::fromStdString (message)});
}
void Application::matchStartCallback ()
{
    //
}
void Application::selectRolePlayerCallback ()
{
    selectRolePlayer ();
}
void Application::joinSpectatorCallback ()
{
    QMessageBox::critical (nullptr, "Join spectator callback not implemented", "Join spectator callback not implemented");
}
void Application::quitCallback ()
{
    exit (0);
}
void Application::authorizationPromptCallback ()
{
    // TODO: Implement DNS resolving
    AuthorizationWidget* authorization_widget = new AuthorizationWidget (loadCredentials ());
    connect (authorization_widget, SIGNAL (windowsCloseRequested ()), this, SLOT (quitCallback ()));
    connect (authorization_widget, SIGNAL (loginRequested (const AuthorizationCredentials&)),
             this, SLOT (loginCallback (const AuthorizationCredentials&)));
    connect (authorization_widget, &AuthorizationWidget::savedCredentialsUpdated, this, &Application::savedCredentials);

    setCurrentWindow (authorization_widget);
}
void Application::loginCallback (const AuthorizationCredentials& credentials) // QString& host, quint16 port, const QString& login, const QString& password)
{
    this->host_address = QHostAddress (credentials.host);
    this->port = credentials.port;
    this->login = credentials.login;

    AuthorizationProgressWidget* authorization_progress_widget = new AuthorizationProgressWidget (
        "Connecting to '" + credentials.host + ":" + QString::number (credentials.port) + "' as '" + credentials.login + "'");
    connect (authorization_progress_widget, SIGNAL (cancelRequested ()), this, SLOT (authorizationPromptCallback ()));
    RTS::Request request_oneof;
    RTS::AuthorizationRequest* request = request_oneof.mutable_authorization ();
    request->set_login (credentials.login.toStdString ());
    request->set_password (credentials.password.toStdString ());

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram ({this->host_address, this->port, {}, request_id++, QByteArray::fromStdString (message)});

    setCurrentWindow (authorization_progress_widget);
}
void Application::createRoomCallback (const QString& name)
{
    if (!session_id.has_value ())
        return;

    RTS::Request request_oneof;
    RTS::CreateRoomRequest* request = request_oneof.mutable_create_room ();
    request->set_name (name.toStdString ());

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, QByteArray::fromStdString (message)});
}
void Application::joinRoomCallback (quint32 room_id)
{
    if (!session_id.has_value ())
        return;

    RTS::Request request_oneof;
    RTS::JoinRoomRequest* request = request_oneof.mutable_join_room ();
    request->set_room_id (room_id);

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, QByteArray::fromStdString (message)});
}
void Application::createUnitCallback (Unit::Team team, Unit::Type type, QPointF position)
{
    if (!session_id.has_value ())
        return;

    RTS::Request request_oneof;
    RTS::UnitCreateRequest* request = request_oneof.mutable_unit_create ();
    request->mutable_position ()->set_x (position.x ());
    request->mutable_position ()->set_y (position.y ());
    RTS::UnitType unit_type = RTS::UnitType::UNIT_TYPE_CRUSADER;
    switch (type) {
    case Unit::Type::Crusader: {
        unit_type = RTS::UnitType::UNIT_TYPE_CRUSADER;
    } break;
    case Unit::Type::Seal: {
        unit_type = RTS::UnitType::UNIT_TYPE_SEAL;
    } break;
    case Unit::Type::Goon: {
        unit_type = RTS::UnitType::UNIT_TYPE_GOON;
    } break;
    case Unit::Type::Beetle: {
        unit_type = RTS::UnitType::UNIT_TYPE_BEETLE;
    } break;
    case Unit::Type::Contaminator: {
        unit_type = RTS::UnitType::UNIT_TYPE_CONTAMINATOR;
    } break;
    default: {
    }
    }
    request->set_unit_type (unit_type);
    request->set_id (10);

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, QByteArray::fromStdString (message)});
}
void Application::savedCredentials (const QVector<AuthorizationCredentials>& credentials)
{
    QSettings settings ("HC Software", "RTS Client");
    {
        settings.beginWriteArray ("credentials");
        for (qsizetype i = 0; i < credentials.size (); ++i) {
            settings.setArrayIndex (i);
            settings.setValue ("host", credentials[i].host);
            settings.setValue ("port", credentials[i].port);
            settings.setValue ("login", credentials[i].login);
            settings.setValue ("password", credentials[i].password);
        }
        settings.endArray ();
    }
}
void Application::unitActionCallback (quint32 id, const UnitActionVariant& action)
{
    if (!session_id.has_value ())
        return;

    RTS::Request request_oneof;
    RTS::UnitActionRequest* request = request_oneof.mutable_unit_action ();
    request->set_unit_id (id);
    RTS::UnitAction* unit_action = request->mutable_action ();
    if (std::holds_alternative<MoveAction> (action)) {
        MoveAction move_action = std::get<MoveAction> (action);
        RTS::MoveAction* move = unit_action->mutable_move ();
        if (move_action.target.index () == 0) {
            move->mutable_position ()->mutable_position ()->set_x (std::get<QPointF> (move_action.target).x ());
            move->mutable_position ()->mutable_position ()->set_y (std::get<QPointF> (move_action.target).y ());
        } else {
            move->mutable_unit ()->set_id (std::get<quint32> (move_action.target));
        }
    } else if (std::holds_alternative<AttackAction> (action)) {
        AttackAction attack_action = std::get<AttackAction> (action);
        RTS::AttackAction* attack = unit_action->mutable_attack ();
        if (attack_action.target.index () == 0) {
            attack->mutable_position ()->mutable_position ()->set_x (std::get<QPointF> (attack_action.target).x ());
            attack->mutable_position ()->mutable_position ()->set_y (std::get<QPointF> (attack_action.target).y ());
        } else {
            attack->mutable_unit ()->set_id (std::get<quint32> (attack_action.target));
        }
    } else if (std::holds_alternative<CastAction> (action)) {
        CastAction cast_action = std::get<CastAction> (action);
        RTS::CastAction* cast = unit_action->mutable_cast ();
        cast->mutable_position ()->mutable_position ()->set_x (cast_action.target.x ());
        cast->mutable_position ()->mutable_position ()->set_y (cast_action.target.y ());
        switch (cast_action.type) {
        case (CastAction::Type::Pestilence): {
            cast->set_type (RTS::CastType::CAST_TYPE_PESTILENCE);
        } break;
        case (CastAction::Type::SpawnBeetle): {
            cast->set_type (RTS::CastType::CAST_TYPE_SPAWN_BEETLE);
        } break;
        default: {
            return;
        }
        }
    } else if (std::holds_alternative<StopAction> (action)) {
        StopAction stop_action = std::get<StopAction> (action);
        RTS::StopAction* stop = unit_action->mutable_stop ();
        if (stop_action.current_target.has_value ()) {
            stop->mutable_target ()->set_id (stop_action.current_target.value ());
        }
    }

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, QByteArray::fromStdString (message)});
}

void Application::sessionDatagramHandler (const QSharedPointer<HCCN::ServerToClient::Message>& message)
{
    const QByteArray& data = message->message;
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
            this->session_id = session_token.value ();
            showLobby (login);

            RTS::Request request_oneof;
            RTS::QueryRoomListRequest* request = request_oneof.mutable_query_room_list ();
            (void) request;
            std::string message;
            request_oneof.SerializeToString (&message);

            network_thread->sendDatagram ({this->host_address, this->port, *this->session_id, request_id++, QByteArray::fromStdString (message)});
        } break;
        default: {
            qDebug () << "Response -> AuthorizationResponse -> UNKNOWN:" << response.response_case ();
        }
        }
    } break;
    case RTS::Response::MessageCase::kSessionClosed: {
        qDebug () << "Response -> SessionClosedResponse -> NOT IMPLEMENTED";
    } break;
    case RTS::Response::MessageCase::kJoinRoom: {
        showRoom ();
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
            room_list.append ({room_info.id (), QString::fromStdString (room_info.name ()),
                               room_info.client_count (), room_info.player_count (), room_info.ready_player_count (), room_info.spectator_count ()});
        }
        emit roomListUpdated (room_list);
    } break;
    case RTS::Response::MessageCase::kSelectRole: {
        // lets manage some shit
        const RTS::SelectRoleResponse& response = response_oneof.select_role ();
        if (response.has_success ()) {
            emit queryReadiness ();
        } else if (response.has_error ()) {
            QMessageBox::critical (nullptr, "Failed to select role at room", QString::fromStdString (response.error ().message ())); // TODO: Apply code
        } else {
            QMessageBox::critical (nullptr, "Failed to select role at room", "Unknown error");
        }
    } break;
    case RTS::Response::MessageCase::kReady: {
    } break;
    case RTS::Response::MessageCase::kMatchPrepared: {
        const RTS::MatchPreparedResponse& response = response_oneof.match_prepared ();
        switch (response.team ()) {
        case RTS::Team::TEAM_RED:
            emit startCountdown (Unit::Team::Red);
            break;
        case RTS::Team::TEAM_BLUE:
            emit startCountdown (Unit::Team::Blue);
            break;
        default:
            QMessageBox::critical (nullptr, "Malformed message from server", "Invalid team at 'MatchStartedResponse'");
        }
    } break;
    case RTS::Response::MessageCase::kMatchStart: {
        emit startMatch ();
    } break;
    case RTS::Response::MessageCase::kMatchState: {
        const RTS::MatchStateResponse& response = response_oneof.match_state ();
        QVector<QPair<quint32, Unit>> units;
        QVector<QPair<quint32, Corpse>> corpses;
        QVector<QPair<quint32, Missile>> missiles;
        QString error_message;
        if (!parseMatchState (response, units, corpses, missiles, error_message)) {
            QMessageBox::critical (nullptr, "Malformed message from server", error_message);
            return;
        }
        emit updateMatchState (units, corpses, missiles);
        last_tick = response.tick ();
    } break;
    case RTS::Response::MessageCase::kError: {
        const RTS::ErrorResponse& response = response_oneof.error ();
        QMessageBox::critical (nullptr, "Malformed message from server", QString::fromStdString (response.error ().message ()));
    } break;
    default: {
        qDebug () << "Response -> UNKNOWN:" << response_oneof.message_case ();
    }
    }
}
void Application::showLobby (const QString& login)
{
    LobbyWidget* lobby_widget = new LobbyWidget (login);
    connect (this, SIGNAL (roomListUpdated (const QVector<RoomEntry>&)), lobby_widget, SLOT (setRoomList (const QVector<RoomEntry>&)));
    connect (lobby_widget, SIGNAL (createRoomRequested (const QString&)), this, SLOT (createRoomCallback (const QString&)));
    connect (lobby_widget, SIGNAL (joinRoomRequested (quint32)), this, SLOT (joinRoomCallback (quint32)));
    setCurrentWindow (lobby_widget);
}
void Application::showRoom (bool single_mode)
{
    RoomWidget* room_widget = new RoomWidget; // connect roomwidget signals ...
    connect (room_widget, &RoomWidget::selectRolePlayerRequested, this, &Application::selectRolePlayerCallback);
    connect (room_widget, &RoomWidget::spectateRequested, this, &Application::joinSpectatorCallback);
    connect (this, &Application::queryReadiness, room_widget, &RoomWidget::readinessHandler);
    connect (room_widget, &RoomWidget::readinessRequested, this, &Application::readinessCallback);
    connect (this, &Application::startMatch, room_widget, &RoomWidget::startMatchHandler);
    connect (this, &Application::startCountdown, room_widget, &RoomWidget::startCountDownHandler);
    connect (this, &Application::updateMatchState, room_widget, &RoomWidget::loadMatchState);
    connect (room_widget, &RoomWidget::createUnitRequested, this, &Application::createUnitCallback);
    connect (room_widget, &RoomWidget::unitActionRequested, this, &Application::unitActionCallback);
    room_widget->grabMouse ();
    room_widget->grabKeyboard ();
    room_widget->showFullScreen ();
    setCurrentWindow (room_widget);

    if (single_mode)
        startSingleMode (room_widget);
}
void Application::startSingleMode (RoomWidget* room_widget)
{
    room_widget->startMatch (Unit::Team::Red);

    quint32 unit_id = 0;
    std::mt19937 random_generator;
    QVector<QPair<quint32, Unit>> units = {
        {unit_id++, {Unit::Type::Seal, random_generator (), Unit::Team::Red, {-20, -15}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Goon, random_generator (), Unit::Team::Red, {-22, -15}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Contaminator, random_generator (), Unit::Team::Red, {-24, -15}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-15, -7}, M_PI * 0.5}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-10, -5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-12, -3}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-11, -2}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-9, -1}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-8, 0}, 0}},
        {unit_id++, {Unit::Type::Seal, random_generator (), Unit::Team::Red, {1, 3}, 0}},
        {unit_id++, {Unit::Type::Seal, random_generator (), Unit::Team::Red, {8, 3}, 0}},
        {unit_id++, {Unit::Type::Seal, random_generator (), Unit::Team::Red, {8, 6}, 0}},
        {unit_id++, {Unit::Type::Seal, random_generator (), Unit::Team::Red, {8, 9}, 0}},
        {unit_id++, {Unit::Type::Seal, random_generator (), Unit::Team::Blue, {10, 5}, 0}},
        {unit_id++, {Unit::Type::Contaminator, random_generator (), Unit::Team::Red, {3, 2}, 0}},
        {unit_id++, {Unit::Type::Contaminator, random_generator (), Unit::Team::Red, {5, 2}, 0}},
        {unit_id++, {Unit::Type::Contaminator, random_generator (), Unit::Team::Red, {7, 2}, 0}},
        {unit_id++, {Unit::Type::Contaminator, random_generator (), Unit::Team::Red, {9, 2}, 0}},
        {unit_id++, {Unit::Type::Goon, random_generator (), Unit::Team::Red, {10, 2}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Red, {-20, -8}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {-4, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {-3, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {-2, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {-1, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {0, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {1, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {2, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {3, 5}, 0}},
        {unit_id++, {Unit::Type::Crusader, random_generator (), Unit::Team::Blue, {4, 5}, 0}},
    };
    for (int off = -4; off <= 4; ++off)
        units.push_back ({unit_id++, {Unit::Type::Seal, random_generator (), Unit::Team::Blue, {off * 2.0 / 3.0, 7}, 0}});
    emit updateMatchState (units, {}, {});
}
bool Application::parseMatchState (const RTS::MatchStateResponse& response,
                                   QVector<QPair<quint32, Unit>>& units, QVector<QPair<quint32, Corpse>>& corpses, QVector<QPair<quint32, Missile>>& missiles,
                                   QString& error_message)
{
    for (int i = 0; i < response.units_size (); i++) {
        std::optional<std::pair<quint32, Unit>> id_unit = parseUnit (response.units (i), error_message);
        if (id_unit == std::nullopt)
            return false;
        units.emplace_back (id_unit->first, id_unit->second);
    }
    for (int i = 0; i < response.corpses_size (); i++) {
        std::optional<std::pair<quint32, Corpse>> id_corpse = parseCorpse (response.corpses (i), error_message);
        if (id_corpse == std::nullopt)
            return false;
        corpses.emplace_back (id_corpse->first, id_corpse->second);
    }
    for (int i = 0; i < response.missiles_size (); i++) {
        std::optional<std::pair<quint32, Missile>> id_missile = parseMissile (response.missiles (i), error_message);
        if (id_missile == std::nullopt)
            return false;
        missiles.emplace_back (id_missile->first, id_missile->second);
    }
    return true;
}
std::optional<std::pair<quint32, Unit>> Application::parseUnit (const RTS::Unit& m_unit, QString& error_message)
{
    Unit::Team team;
    switch (m_unit.team ()) {
    case RTS::Team::TEAM_RED:
        team = Unit::Team::Red;
        break;
    case RTS::Team::TEAM_BLUE:
        team = Unit::Team::Blue;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid unit team";
        return std::nullopt;
    }

    Unit::Type type;
    switch (m_unit.type ()) {
    case RTS::UnitType::UNIT_TYPE_CRUSADER:
        type = Unit::Type::Crusader;
        break;
    case RTS::UnitType::UNIT_TYPE_SEAL:
        type = Unit::Type::Seal;
        break;
    case RTS::UnitType::UNIT_TYPE_GOON:
        type = Unit::Type::Goon;
        break;
    case RTS::UnitType::UNIT_TYPE_BEETLE:
        type = Unit::Type::Beetle;
        break;
    case RTS::UnitType::UNIT_TYPE_CONTAMINATOR:
        type = Unit::Type::Contaminator;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid unit type";
        return std::nullopt;
    }

    if (!m_unit.has_position ()) {
        error_message = "Invalid 'MatchState' from server: missing position";
        return std::nullopt;
    }

    quint32 id = m_unit.has_client_id () ? m_unit.client_id ().id () : m_unit.id ();
    Unit unit = Unit (type, 0, team, QPointF (m_unit.position ().x (), m_unit.position ().y ()), m_unit.orientation ());
    unit.hp = m_unit.health ();
    if (!parseUnitAction (m_unit.current_action (), unit.action, error_message))
        return std::nullopt;
    unit.attack_cooldown_left_ticks = m_unit.attack_cooldown_left_ticks ();
    unit.cast_cooldown_left_ticks = m_unit.cast_cooldown_left_ticks ();
    return std::pair<quint32, Unit> (id, unit);
}
std::optional<std::pair<quint32, Corpse>> Application::parseCorpse (const RTS::Corpse& m_corpse, QString& error_message)
{
    if (!m_corpse.has_unit ())
        return std::nullopt;
    std::optional<std::pair<quint32, Unit>> parsed_unit = parseUnit (m_corpse.unit (), error_message);
    if (!parsed_unit.has_value ())
        return std::nullopt;
    Corpse corpse (parsed_unit->second, m_corpse.decay_remaining_ticks ());
    return std::pair<quint32, Corpse> (parsed_unit->first, corpse);
}
std::optional<std::pair<quint32, Missile>> Application::parseMissile (const RTS::Missile& m_missile, QString& error_message)
{
    Unit::Team team;
    switch (m_missile.team ()) {
    case RTS::Team::TEAM_RED:
        team = Unit::Team::Red;
        break;
    case RTS::Team::TEAM_BLUE:
        team = Unit::Team::Blue;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid missile sender team";
        return std::nullopt;
    }

    Missile::Type type;
    switch (m_missile.type ()) {
    case RTS::MissileType::MISSILE_ROCKET:
        type = Missile::Type::Rocket;
        break;
    case RTS::MissileType::MISSILE_PESTILENCE:
        type = Missile::Type::Pestilence;
        break;
    default:
        error_message = "Invalid 'MatchState' from server: invalid missile type";
        return std::nullopt;
    }

    if (!m_missile.has_position ()) {
        error_message = "Invalid 'MatchState' from server: missing missile position";
        return std::nullopt;
    }

    quint32 id = m_missile.id ();
    Missile missile (type, team, QPointF (m_missile.position ().x (), m_missile.position ().y ()), 0, QPointF (m_missile.target_position ().x (), m_missile.target_position ().y ()));

    if (m_missile.has_target_unit ())
        missile.target_unit = m_missile.target_unit ().id ();
    else
        missile.target_unit.reset ();

    return std::pair<quint32, Missile> (id, missile);
}
bool Application::parseUnitAction (const RTS::UnitAction& m_current_action, UnitActionVariant& unit_action, QString& error_message)
{
    switch (m_current_action.action_case ()) {
    case RTS::UnitAction::kStop: {
        unit_action = parseUnitActionStop (m_current_action.stop ());
    } break;
    case RTS::UnitAction::kMove: {
        unit_action = parseUnitActionMove (m_current_action.move ());
    } break;
    case RTS::UnitAction::kAttack: {
        unit_action = parseUnitActionAttack (m_current_action.attack ());
    } break;
    case RTS::UnitAction::kCast: {
        std::optional<CastAction> cast = parseUnitActionCast (m_current_action.cast (), error_message);
        if (cast == std::nullopt)
            return false;
        unit_action = *cast;
    } break;
    case RTS::UnitAction::kPerformingAttack: {
        std::optional<PerformingAttackAction> performing_attack = parseUnitActionPerformingAttack (m_current_action.performing_attack (), error_message);
        if (performing_attack == std::nullopt)
            return false;
        unit_action = *performing_attack;
    } break;
    case RTS::UnitAction::kPerformingCast: {
        std::optional<PerformingCastAction> performing_cast = parseUnitActionPerformingCast (m_current_action.performing_cast (), error_message);
        if (performing_cast == std::nullopt)
            return false;
        unit_action = *performing_cast;
    } break;
    default: {
        error_message = "Invalid unit action type from server";
    } return false;
    }
    return true;
}
StopAction Application::parseUnitActionStop (const RTS::StopAction& m_stop_action)
{
    StopAction stop;
    if (m_stop_action.has_target ())
        stop.current_target = m_stop_action.target ().id ();
    return stop;
}
MoveAction Application::parseUnitActionMove (const RTS::MoveAction& m_move_action)
{
    MoveAction move (QPointF (0, 0));
    if (m_move_action.target_case () == RTS::MoveAction::kPosition)
        move.target = QPointF (m_move_action.position ().position ().x (), m_move_action.position ().position ().y ());
    else
        move.target = (quint32) m_move_action.unit ().id ();
    return move;
}
AttackAction Application::parseUnitActionAttack (const RTS::AttackAction& m_attack_action)
{
    AttackAction attack (QPointF (0, 0));
    if (m_attack_action.target_case () == RTS::AttackAction::kPosition)
        attack.target = QPointF (m_attack_action.position ().position ().x (), m_attack_action.position ().position ().y ());
    else
        attack.target = (quint32) m_attack_action.unit ().id ();
    return attack;
}
std::optional<CastAction> Application::parseUnitActionCast (const RTS::CastAction& m_cast_action, QString& error_message)
{
    CastAction cast (CastAction::Type::Unknown, QPointF (0, 0));
    cast.target = QPointF (m_cast_action.position ().position ().x (), m_cast_action.position ().position ().y ());
    switch (m_cast_action.type ()) {
    case RTS::CastType::CAST_TYPE_PESTILENCE:
        cast.type = CastAction::Type::Pestilence;
        break;
    case RTS::CastType::CAST_TYPE_SPAWN_BEETLE:
        cast.type = CastAction::Type::SpawnBeetle;
        break;
    default:
        error_message = "Invalid unit action cast type from server";
        return std::nullopt;
    }
    return cast;
}
std::optional<PerformingAttackAction> Application::parseUnitActionPerformingAttack (const RTS::PerformingAttackAction& m_peforming_attack_action, QString& error_message)
{
    qint64 remaining_ticks = m_peforming_attack_action.remaining_ticks ();
    switch (m_peforming_attack_action.next_action_case ()) {
    case RTS::UnitAction::kStop: {
        return PerformingAttackAction (parseUnitActionStop (m_peforming_attack_action.stop ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kMove: {
        return PerformingAttackAction (parseUnitActionMove (m_peforming_attack_action.move ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kAttack: {
        return PerformingAttackAction (parseUnitActionAttack (m_peforming_attack_action.attack ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kCast: {
        std::optional<CastAction> cast = parseUnitActionCast (m_peforming_attack_action.cast (), error_message);
        if (cast == std::nullopt)
            return std::nullopt;
        return PerformingAttackAction (*cast, remaining_ticks);
    } break;
    default: {
        error_message = "Invalid unit next action type from server";
        return std::nullopt;
    }
    }
}
std::optional<PerformingCastAction> Application::parseUnitActionPerformingCast (const RTS::PerformingCastAction& m_peforming_cast_action, QString& error_message)
{
    qint64 remaining_ticks = m_peforming_cast_action.remaining_ticks ();
    CastAction::Type cast_type;
    switch (m_peforming_cast_action.cast_type ()) {
    case RTS::CastType::CAST_TYPE_PESTILENCE:
        cast_type = CastAction::Type::Pestilence;
        break;
    case RTS::CastType::CAST_TYPE_SPAWN_BEETLE:
        cast_type = CastAction::Type::SpawnBeetle;
        break;
    default:
        error_message = "Invalid unit next action cast type from server";
        return std::nullopt;
    }
    switch (m_peforming_cast_action.next_action_case ()) {
    case RTS::UnitAction::kStop: {
        return PerformingCastAction (cast_type, parseUnitActionStop (m_peforming_cast_action.stop ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kMove: {
        return PerformingCastAction (cast_type, parseUnitActionMove (m_peforming_cast_action.move ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kAttack: {
        return PerformingCastAction (cast_type, parseUnitActionAttack (m_peforming_cast_action.attack ()), remaining_ticks);
    } break;
    case RTS::UnitAction::kCast: {
        std::optional<CastAction> cast = parseUnitActionCast (m_peforming_cast_action.cast (), error_message);
        if (cast == std::nullopt)
            return std::nullopt;
        return PerformingCastAction (cast_type, *cast, remaining_ticks);
    } break;
    default: {
        error_message = "Invalid unit next action type from server 2";
        return std::nullopt;
    }
    }
}
QVector<AuthorizationCredentials> Application::loadCredentials ()
{
    QVector<AuthorizationCredentials> credentials;
    QSettings settings ("HC Software", "RTS Client");
    {
        int size = settings.beginReadArray ("credentials");
        for (qsizetype i = 0; i < size; ++i) {
            settings.setArrayIndex (i);
            credentials.append ({
                settings.value ("host").toString (),
                quint16 (settings.value ("port").toInt ()),
                settings.value ("login").toString (),
                settings.value ("password").toString (),
            });
        }
        settings.endArray ();
    }
    return credentials;
}
void Application::setCurrentWindow (QWidget* new_window)
{
    if (current_window)
        current_window->deleteLater ();
    new_window->setWindowIcon (QIcon (":/images/application-icon.png"));
    new_window->show ();
    current_window = new_window;
}
