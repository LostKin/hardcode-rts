#include "application.h"

#include "mainwindow.h"
#include "screens/authorizationscreen.h"
#include "screens/authorizationprogressscreen.h"
#include "screens/lobbyscreen.h"
#include "screens/roleselectionscreen.h"
#include "screens/roleselectionprogressscreen.h"
#include "screens/readinessscreen.h"
#include "matchstate.h"
#include "parse.h"
#include "singlemodeloader.h"
#include "requests.pb.h"

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

    network_thread.reset (new NetworkThread (this));
    connect (&*network_thread, &NetworkThread::datagramReceived, this, &Application::sessionDatagramHandler);
}
Application::~Application ()
{
    if (main_window)
        main_window->deleteLater ();

    network_thread->exit ();
    network_thread->wait ();
}

void Application::start ()
{
    network_thread->start ();

    main_window = new MainWindow;
    main_window->setWindowIcon (QIcon (":/images/application-icon.png"));
    main_window->showFullScreen ();

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
    std::string message = request_oneof.SerializeAsString ();
    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, {message.data (), message.data () + message.size ()}});
}
void Application::readinessCallback ()
{
    RTS::Request request_oneof;
    RTS::ReadyRequest* request = request_oneof.mutable_ready ();
    (void) request;
    std::string message = request_oneof.SerializeAsString ();
    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, {message.data (), message.data () + message.size ()}});
}
void Application::matchStartCallback ()
{
    //
}
void Application::selectRolePlayerCallback ()
{
    showRoleSelectionProgressScreen ();
    selectRolePlayer ();
}
void Application::joinSpectatorCallback ()
{
    QMessageBox::critical (nullptr, "Join spectator callback not implemented", "Join spectator callback not implemented");
}
void Application::quitCallback ()
{
    QMessageBox::critical (nullptr, "Quit callback not implemented", "Quit callback not implemented");
}
void Application::authorizationPromptCallback ()
{
    // TODO: Implement DNS resolving
    AuthorizationScreen* authorization_screen = new AuthorizationScreen (loadCredentials ());
    connect (authorization_screen, SIGNAL (windowsCloseRequested ()), this, SLOT (quitCallback ()));
    connect (authorization_screen, SIGNAL (loginRequested (const AuthorizationCredentials&)),
             this, SLOT (loginCallback (const AuthorizationCredentials&)));
    connect (authorization_screen, &AuthorizationScreen::savedCredentialsUpdated, this, &Application::savedCredentials);

    setCurrentWindow (authorization_screen);
}
void Application::loginCallback (const AuthorizationCredentials& credentials) // QString& host, quint16 port, const QString& login, const QString& password)
{
    this->host_address = QHostAddress (credentials.host);
    this->port = credentials.port;
    this->login = credentials.login;

    AuthorizationProgressScreen* authorization_progress_screen = new AuthorizationProgressScreen (
        "Connecting to '" + credentials.host + ":" + QString::number (credentials.port) + "' as '" + credentials.login + "'");
    connect (authorization_progress_screen, SIGNAL (cancelRequested ()), this, SLOT (authorizationPromptCallback ()));
    RTS::Request request_oneof;
    RTS::AuthorizationRequest* request = request_oneof.mutable_authorization ();
    request->set_login (credentials.login.toStdString ());
    request->set_password (credentials.password.toStdString ());

    std::string message;
    request_oneof.SerializeToString (&message);

    network_thread->sendDatagram ({this->host_address, this->port, {}, request_id++, {message.data (), message.data () + message.size ()}});

    setCurrentWindow (authorization_progress_screen);
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

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, {message.data (), message.data () + message.size ()}});
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

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, {message.data (), message.data () + message.size ()}});
}
void Application::roleSelectedCallback ()
{
    showReadinessScreen ();
}
void Application::createUnitCallback (Unit::Team /* team */, Unit::Type type, const Position& position)
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

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, {message.data (), message.data () + message.size ()}});
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
            move->mutable_position ()->mutable_position ()->set_x (std::get<Position> (move_action.target).x ());
            move->mutable_position ()->mutable_position ()->set_y (std::get<Position> (move_action.target).y ());
        } else {
            move->mutable_unit ()->set_id (std::get<quint32> (move_action.target));
        }
    } else if (std::holds_alternative<AttackAction> (action)) {
        AttackAction attack_action = std::get<AttackAction> (action);
        RTS::AttackAction* attack = unit_action->mutable_attack ();
        if (attack_action.target.index () == 0) {
            attack->mutable_position ()->mutable_position ()->set_x (std::get<Position> (attack_action.target).x ());
            attack->mutable_position ()->mutable_position ()->set_y (std::get<Position> (attack_action.target).y ());
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

    network_thread->sendDatagram ({this->host_address, this->port, session_id.value (), request_id++, {message.data (), message.data () + message.size ()}});
}

void Application::sessionDatagramHandler (const std::shared_ptr<HCCN::ServerToClient::Message>& message)
{
    RTS::Response response_oneof;
    if (!response_oneof.ParseFromArray (message->message.data (), message->message.size ())) {
        log ("Invalid response from server: failed to parse protobuf message");
        return;
    }

    switch (response_oneof.message_case ()) {
    case RTS::Response::MessageCase::kAuthorization: {
        const RTS::AuthorizationResponse& response = response_oneof.authorization ();
        switch (response.response_case ()) {
        case RTS::AuthorizationResponse::kError: {
            const RTS::Error& error = response.error ();
            log ("Response -> AuthorizationResponse -> ERROR: " + QString::fromStdString (error.message ()));
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

            network_thread->sendDatagram ({this->host_address, this->port, *this->session_id, request_id++, {message.data (), message.data () + message.size ()}});
        } break;
        default: {
            QString message = "Response -> AuthorizationResponse -> UNKNOWN #" + QString::number (response.response_case ());
            QMessageBox::critical (nullptr, message, message);
        }
        }
    } break;
    case RTS::Response::MessageCase::kSessionClosed: {
        QMessageBox::critical (nullptr, "Response -> SessionClosedResponse -> NOT IMPLEMENTED", "Response -> SessionClosedResponse -> NOT IMPLEMENTED");
    } break;
    case RTS::Response::MessageCase::kJoinRoom: {
        showRoleSelectionScreen ();
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
            roleSelectedCallback ();
        } else if (response.has_error ()) {
            QMessageBox::critical (nullptr, "Failed to select role at room", QString::fromStdString (response.error ().message ())); // TODO: Apply code
        } else {
            QMessageBox::critical (nullptr, "Failed to select role at room", "Unknown error");
        }
    } break;
    case RTS::Response::MessageCase::kReady: {
    } break;
    case RTS::Response::MessageCase::kMatchPrepared: {
        showRoom ();
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
        std::vector<std::pair<quint32, Unit>> units;
        std::vector<std::pair<quint32, Corpse>> corpses;
        std::vector<std::pair<quint32, Missile>> missiles;
        std::string error_message;
        if (!RTSN::Parse::matchState (response, units, corpses, missiles, error_message)) {
            QMessageBox::critical (nullptr, "Malformed message from server", QString::fromStdString (error_message));
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
        QString message = "Response -> UNKNOWN #" + QString::number (response_oneof.message_case ());
        QMessageBox::critical (nullptr, message, message);
    }
    }
}
void Application::showLobby (const QString& login)
{
    LobbyScreen* lobby_screen = new LobbyScreen (login);
    connect (this, SIGNAL (roomListUpdated (const QVector<RoomEntry>&)), lobby_screen, SLOT (setRoomList (const QVector<RoomEntry>&)));
    connect (lobby_screen, SIGNAL (createRoomRequested (const QString&)), this, SLOT (createRoomCallback (const QString&)));
    connect (lobby_screen, SIGNAL (joinRoomRequested (quint32)), this, SLOT (joinRoomCallback (quint32)));
    setCurrentWindow (lobby_screen);
}
void Application::showRoleSelectionScreen ()
{
    RoleSelectionScreen* next_screen = new RoleSelectionScreen;
    connect (next_screen, &RoleSelectionScreen::selectRolePlayerRequested, this, &Application::selectRolePlayerCallback);
    connect (next_screen, &RoleSelectionScreen::spectateRequested, this, &Application::joinSpectatorCallback);
    // TODO: Connect cancel
    setCurrentWindow (next_screen);
}
void Application::showRoleSelectionProgressScreen ()
{
    RoleSelectionProgressScreen* next_screen = new RoleSelectionProgressScreen;
    // TODO: Connect signals
    setCurrentWindow (next_screen);
}
void Application::showReadinessScreen ()
{
    ReadinessScreen* next_screen = new ReadinessScreen;
    connect (next_screen, &ReadinessScreen::readinessRequested, this, &Application::readinessCallback);
    // TODO: Connect signals
    setCurrentWindow (next_screen);
}
void Application::showRoom (bool single_mode)
{
    RoomWidget* room_widget = new RoomWidget; // connect roomwidget signals ...
    connect (this, &Application::startMatch, room_widget, &RoomWidget::startMatchHandler);
    connect (this, &Application::startCountdown, room_widget, &RoomWidget::startCountDownHandler);
    connect (this, &Application::updateMatchState, room_widget, &RoomWidget::loadMatchState);
    connect (room_widget, &RoomWidget::createUnitRequested, this, &Application::createUnitCallback);
    connect (room_widget, &RoomWidget::unitActionRequested, this, &Application::unitActionCallback);
    connect (this, &Application::log, room_widget, &RoomWidget::log);
    setCurrentWindow (room_widget, true);

    if (single_mode)
        startSingleMode (room_widget);
}
void Application::startSingleMode (RoomWidget* room_widget)
{
    room_widget->startMatch (Unit::Team::Red);

    std::vector<std::pair<quint32, Unit>> units;
    std::vector<std::pair<quint32, Corpse>> corpses;
    std::vector<std::pair<quint32, Missile>> missiles;
    SingleModeLoader::load (units, corpses, missiles);

    emit updateMatchState (units, corpses, missiles);
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
void Application::setCurrentWindow (QWidget* new_window, bool fullscreen)
{
    if (fullscreen) {
        new_window->grabMouse ();
        new_window->grabKeyboard ();
    }
    main_window->setWidget (new_window);
}
