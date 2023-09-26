#include "application.h"
#include "authorizationwidget.h"
#include "authorizationprogresswidget.h"
#include "lobbywidget.h"
#include "matchstate.h"

#include <QDebug>
#include <QDateTime>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSettings>

MatchStateCollector::MatchStateCollector (const RTS::MatchStateFragmentResponse& initial_fragment)
{
    if (initial_fragment.fragment_count () > 0 && initial_fragment.fragment_count () <= 1024 && initial_fragment.fragment_no () < initial_fragment.fragment_count ()) {
        fragments_.resize (initial_fragment.fragment_count ());
        fragments_[initial_fragment.fragment_no ()] = QSharedPointer<RTS::MatchStateFragmentResponse> (new RTS::MatchStateFragmentResponse (initial_fragment));
        filled_fragment_count = 1;
    } else {
        filled_fragment_count = 0;
    }
}
bool MatchStateCollector::addFragment (const RTS::MatchStateFragmentResponse& fragment)
{
    if (fragment.fragment_no () >= quint32 (fragments_.size ())) {
        return false;
    }
    if (!fragments_[fragment.fragment_no ()]) {
        fragments_[fragment.fragment_no ()] = QSharedPointer<RTS::MatchStateFragmentResponse> (new RTS::MatchStateFragmentResponse (fragment));
        ++filled_fragment_count;
    }
    return true;
}
bool MatchStateCollector::valid () const
{
    return fragments_.size () > 0;
}
bool MatchStateCollector::filled () const
{
    return filled_fragment_count == quint32 (fragments_.size ());
}
const QVector<QSharedPointer<RTS::MatchStateFragmentResponse>>& MatchStateCollector::fragments () const
{
    return fragments_;
}

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
void Application::unitActionCallback (quint32 id, const std::variant<StopAction, MoveAction, AttackAction, CastAction>& action)
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
            cast->set_type (RTS::CastType::PESTILENCE);
        } break;
        case (CastAction::Type::SpawnBeetle): {
            cast->set_type (RTS::CastType::SPAWN_BEETLE);
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
    case RTS::Response::MessageCase::kMatchStateFragment: {
        const RTS::MatchStateFragmentResponse& response = response_oneof.match_state_fragment ();
        if (response.fragment_count () == 1) {
            QVector<QPair<quint32, Unit>> units;
            QVector<QPair<quint32, Missile>> missiles;
            QString error_message;
            if (!parseMatchStateFragment (response, units, missiles, error_message)) {
                QMessageBox::critical (nullptr, "Malformed message from server", error_message);
                return;
            }
            emit updateMatchState (units, missiles);
            last_tick = response.tick ();
        } else if (response.tick () > last_tick) {
            QHash<quint32, QSharedPointer<MatchStateCollector>>::iterator it = match_state_collectors.find (response.tick ());
            if (it == match_state_collectors.end ()) {
                QSharedPointer<MatchStateCollector> match_state_collector = QSharedPointer<MatchStateCollector> (new MatchStateCollector (response));
                if (!match_state_collector->valid ()) {
                    QMessageBox::critical (nullptr, "Malformed message from server", "Invalid 'MatchState' from server: invalid fragment info");
                    return;
                }
                match_state_collectors[response.tick ()] = match_state_collector;
            } else {
                if (!(*it)->addFragment (response)) {
                    QMessageBox::critical (nullptr, "Malformed message from server", "Invalid 'MatchState' from server: invalid fragment info");
                    return;
                }
                if ((*it)->filled ()) {
                    QVector<QPair<quint32, Unit>> units;
                    QVector<QPair<quint32, Missile>> missiles;
                    const QVector<QSharedPointer<RTS::MatchStateFragmentResponse>>& fragments = (*it)->fragments ();
                    for (const QSharedPointer<RTS::MatchStateFragmentResponse>& fragment: fragments) {
                        QString error_message;
                        if (!parseMatchStateFragment (*fragment, units, missiles, error_message)) {
                            QMessageBox::critical (nullptr, "Malformed message from server", error_message);
                            return;
                        }
                    }
                    emit updateMatchState (units, missiles);
                }
            }
        }
        {
            QHash<quint32, QSharedPointer<MatchStateCollector>>::iterator it = match_state_collectors.begin ();
            while (it != match_state_collectors.end ()) {
                if (it.key () <= last_tick)
                    it = match_state_collectors.erase (it);
                else
                    ++it;
            }
        }
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
    emit updateMatchState (units, {});
}
bool Application::parseMatchStateFragment (const RTS::MatchStateFragmentResponse& response, QVector<QPair<quint32, Unit>>& units, QVector<QPair<quint32, Missile>>& missiles, QString& error_message)
{
    for (int i = 0; i < response.units_size (); i++) {
        const RTS::Unit& r_unit = response.units (i);
        Unit::Team team;
        switch (r_unit.team ()) {
        case RTS::Team::TEAM_RED:
            team = Unit::Team::Red;
            break;
        case RTS::Team::TEAM_BLUE:
            team = Unit::Team::Blue;
            break;
        default:
            error_message = "Invalid 'MatchState' from server: invalid unit team";
            return false;
        }

        Unit::Type type;
        switch (r_unit.type ()) {
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
            return false;
        }

        if (!r_unit.has_position ()) {
            error_message = "Invalid 'MatchState' from server: missing position";
            return false;
        }

        quint32 id = r_unit.has_client_id () ? r_unit.client_id ().id () : r_unit.id ();
        units.push_back ({id, {type, 0, team, QPointF (r_unit.position ().x (), r_unit.position ().y ()), r_unit.orientation ()}});
        units.last ().second.hp = r_unit.health ();
        switch (r_unit.current_action ().action_case ()) {
        case RTS::UnitAction::kStop: {
            // TODO: Stop selected only
            StopAction stop;
            if (r_unit.current_action ().stop ().has_target ()) {
                stop.current_target = r_unit.current_action ().stop ().target ().id ();
            }
            units.last ().second.action = stop;
        } break;
        case RTS::UnitAction::kMove: {
            MoveAction move (QPointF (0, 0));
            if (r_unit.current_action ().move ().target_case () == RTS::MoveAction::kPosition) {
                move.target = QPointF (r_unit.current_action ().move ().position ().position ().x (), r_unit.current_action ().move ().position ().position ().y ());
            } else {
                move.target = (quint32) r_unit.current_action ().move ().unit ().id ();
            }
            units.last ().second.action = move;
        } break;
        case RTS::UnitAction::kAttack: {
            AttackAction attack (QPointF (0, 0));
            if (r_unit.current_action ().attack ().target_case () == RTS::AttackAction::kPosition) {
                attack.target = QPointF (r_unit.current_action ().attack ().position ().position ().x (), r_unit.current_action ().attack ().position ().position ().y ());
            } else {
                attack.target = (quint32) r_unit.current_action ().attack ().unit ().id ();
            }
            units.last ().second.action = attack;
        } break;
        case RTS::UnitAction::kCast: {
            CastAction cast (CastAction::Type::Unknown, QPointF (0, 0));
            cast.target = QPointF (r_unit.current_action ().cast ().position ().position ().x (), r_unit.current_action ().cast ().position ().position ().y ());
            switch (r_unit.current_action ().cast ().type ()) {
            case RTS::CastType::PESTILENCE: {
                cast.type = CastAction::Type::Pestilence;
            } break;
            case RTS::CastType::SPAWN_BEETLE: {
                cast.type = CastAction::Type::SpawnBeetle;
            } break;
            }
            units.last ().second.action = cast;
        } break;
        }
        units.last ().second.attack_remaining_ticks = r_unit.attack_remaining_ticks ();
        units.last ().second.cast_cooldown_left_ticks = r_unit.cooldown ();
    }
    for (int i = 0; i < response.missiles_size (); i++) {
        const RTS::Missile& r_missile = response.missiles (i);

        Unit::Team team;
        switch (r_missile.team ()) {
        case RTS::Team::TEAM_RED:
            team = Unit::Team::Red;
            break;
        case RTS::Team::TEAM_BLUE:
            team = Unit::Team::Blue;
            break;
        default:
            error_message = "Invalid 'MatchState' from server: invalid missile sender team";
            return false;
        }

        Missile::Type type;
        switch (r_missile.type ()) {
        case RTS::MissileType::MISSILE_ROCKET:
            type = Missile::Type::Rocket;
            break;
        case RTS::MissileType::MISSILE_PESTILENCE:
            type = Missile::Type::Pestilence;
            break;
        default:
            error_message = "Invalid 'MatchState' from server: invalid missile type";
            return false;
        }

        if (!r_missile.has_position ()) {
            error_message = "Invalid 'MatchState' from server: missing missile position";
            return false;
        }

        missiles.push_back ({r_missile.id (), {type, team, QPointF (r_missile.position ().x (), r_missile.position ().y ()), 0, QPointF (r_missile.target_position ().x (), r_missile.target_position ().y ())}});

        if (r_missile.has_target_unit ()) {
            // qDebug() << "Application Missile has target unit";
            missiles.last ().second.target_unit = r_missile.target_unit ().id ();
        } else {
            missiles.last ().second.target_unit.reset ();
        }
    }
    return true;
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
    new_window->show ();
    current_window = new_window;
}
