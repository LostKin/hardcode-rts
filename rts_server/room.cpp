#include "room.h"

#include <QThread>
#include <QUdpSocket>
#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QTimer>

static constexpr quint32 kTickDurationMs = 20;

Room::Room (QObject* parent)
    : QObject (parent)
{
    timer.reset (new QTimer (this));
    connect (&*timer, &QTimer::timeout, this, &Room::tick);
}

void Room::tick ()
{
    quint32 tick_no = match_state->get_tick_no ();
    match_state->tick ();
    RTS::Response response_for_red_oneof;
    RTS::Response response_for_blue_oneof;
    RTS::MatchStateResponse* response_for_red = response_for_red_oneof.mutable_match_state ();
    RTS::MatchStateResponse* response_for_blue = response_for_blue_oneof.mutable_match_state ();
    response_for_red->set_tick (tick_no);
    response_for_blue->set_tick (tick_no);

    for (QHash<quint32, Unit>::const_iterator u_iter = match_state->unitsRef ().cbegin (); u_iter != match_state->unitsRef ().cend (); u_iter++) {
        RTS::Unit* unit_for_red = response_for_red->add_units ();
        RTS::Unit* unit_for_blue = response_for_blue->add_units ();
        if (u_iter->team == Unit::Team::Red) {
            unit_for_red->set_team (RTS::Team::TEAM_RED);
            unit_for_blue->set_team (RTS::Team::TEAM_RED);
            QMap<quint32, quint32>::const_iterator it = red_client_to_server.find (u_iter.key ());
            if (it != red_client_to_server.cend ()) {
                unit_for_red->mutable_client_id ()->set_id (it.key ());
            }
        }
        if (u_iter->team == Unit::Team::Blue) {
            unit_for_red->set_team (RTS::Team::TEAM_BLUE);
            unit_for_blue->set_team (RTS::Team::TEAM_BLUE);
            QMap<quint32, quint32>::const_iterator it = blue_client_to_server.find (u_iter.key ());
            if (it != blue_client_to_server.cend ()) {
                unit_for_blue->mutable_client_id ()->set_id (it.key ());
            }
        }
        switch (u_iter->type) {
        case Unit::Type::Seal: {
            unit_for_red->set_type (RTS::UnitType::UNIT_TYPE_SEAL);
            unit_for_blue->set_type (RTS::UnitType::UNIT_TYPE_SEAL);
        } break;
        case Unit::Type::Crusader: {
            unit_for_red->set_type (RTS::UnitType::UNIT_TYPE_CRUSADER);
            unit_for_blue->set_type (RTS::UnitType::UNIT_TYPE_CRUSADER);
        } break;
        case Unit::Type::Goon: {
            unit_for_red->set_type (RTS::UnitType::UNIT_TYPE_GOON);
            unit_for_blue->set_type (RTS::UnitType::UNIT_TYPE_GOON);
        } break;
        case Unit::Type::Beetle: {
            unit_for_red->set_type (RTS::UnitType::UNIT_TYPE_BEETLE);
            unit_for_blue->set_type (RTS::UnitType::UNIT_TYPE_BEETLE);
        } break;
        case Unit::Type::Contaminator: {
            unit_for_red->set_type (RTS::UnitType::UNIT_TYPE_CONTAMINATOR);
            unit_for_blue->set_type (RTS::UnitType::UNIT_TYPE_CONTAMINATOR);
        } break;
        }
        if (std::holds_alternative<StopAction> (u_iter->action)) {
            if (std::get<StopAction> (u_iter->action).current_target.has_value ()) {
                unit_for_red->mutable_current_action ()->mutable_stop ()->mutable_target ()->set_id (std::get<StopAction> (u_iter->action).current_target.value ());
                unit_for_blue->mutable_current_action ()->mutable_stop ()->mutable_target ()->set_id (std::get<StopAction> (u_iter->action).current_target.value ());
            } else {
                unit_for_red->mutable_current_action ()->mutable_stop ();
                unit_for_blue->mutable_current_action ()->mutable_stop ();
            }
        } else if (std::holds_alternative<MoveAction> (u_iter->action)) {
            if (std::holds_alternative<QPointF> (std::get<MoveAction> (u_iter->action).target)) {
                QPointF position = std::get<QPointF> (std::get<MoveAction> (u_iter->action).target);
                unit_for_red->mutable_current_action ()->mutable_move ()->mutable_position ()->mutable_position ()->set_x (position.x ());
                unit_for_red->mutable_current_action ()->mutable_move ()->mutable_position ()->mutable_position ()->set_y (position.y ());
                unit_for_blue->mutable_current_action ()->mutable_move ()->mutable_position ()->mutable_position ()->set_x (position.x ());
                unit_for_blue->mutable_current_action ()->mutable_move ()->mutable_position ()->mutable_position ()->set_y (position.y ());
            } else {
                quint32 id = std::get<quint32> (std::get<MoveAction> (u_iter->action).target);
                unit_for_red->mutable_current_action ()->mutable_move ()->mutable_unit ()->set_id (id);
                unit_for_blue->mutable_current_action ()->mutable_move ()->mutable_unit ()->set_id (id);
            }
        } else if (std::holds_alternative<AttackAction> (u_iter->action)) {
            if (std::holds_alternative<QPointF> (std::get<AttackAction> (u_iter->action).target)) {
                QPointF position = std::get<QPointF> (std::get<AttackAction> (u_iter->action).target);
                unit_for_red->mutable_current_action ()->mutable_attack ()->mutable_position ()->mutable_position ()->set_x (position.x ());
                unit_for_red->mutable_current_action ()->mutable_attack ()->mutable_position ()->mutable_position ()->set_y (position.y ());
                unit_for_blue->mutable_current_action ()->mutable_attack ()->mutable_position ()->mutable_position ()->set_x (position.x ());
                unit_for_blue->mutable_current_action ()->mutable_attack ()->mutable_position ()->mutable_position ()->set_y (position.y ());
            } else {
                quint32 id = std::get<quint32> (std::get<AttackAction> (u_iter->action).target);
                unit_for_red->mutable_current_action ()->mutable_attack ()->mutable_unit ()->set_id (id);
                unit_for_blue->mutable_current_action ()->mutable_attack ()->mutable_unit ()->set_id (id);
            }
        } else if (std::holds_alternative<CastAction> (u_iter->action)) {
            RTS::CastType type;
            switch (std::get<CastAction> (u_iter->action).type) {
            case CastAction::Type::Pestilence: {
                type = RTS::CastType::PESTILENCE;
            } break;
            case CastAction::Type::SpawnBeetle: {
                type = RTS::CastType::SPAWN_BEETLE;
            } break;
            }
            QPointF position = std::get<CastAction> (u_iter->action).target;
            unit_for_red->mutable_current_action ()->mutable_cast ()->mutable_position ()->mutable_position ()->set_x (position.x ());
            unit_for_red->mutable_current_action ()->mutable_cast ()->mutable_position ()->mutable_position ()->set_y (position.y ());
            unit_for_blue->mutable_current_action ()->mutable_cast ()->mutable_position ()->mutable_position ()->set_x (position.x ());
            unit_for_blue->mutable_current_action ()->mutable_cast ()->mutable_position ()->mutable_position ()->set_y (position.y ());
            unit_for_red->mutable_current_action ()->mutable_cast ()->set_type (type);
            unit_for_blue->mutable_current_action ()->mutable_cast ()->set_type (type);
        }
        unit_for_red->set_attack_remaining_ticks (u_iter->attack_remaining_ticks);
        unit_for_red->set_cooldown (u_iter->cast_cooldown_left_ticks);
        unit_for_blue->set_attack_remaining_ticks (u_iter->attack_remaining_ticks);
        unit_for_blue->set_cooldown (u_iter->cast_cooldown_left_ticks);

        unit_for_red->mutable_position ()->set_x (u_iter->position.x ());
        unit_for_red->mutable_position ()->set_y (u_iter->position.y ());
        unit_for_red->set_health (u_iter->hp);
        unit_for_red->set_orientation (u_iter->orientation);
        unit_for_red->set_id (u_iter.key ());
        unit_for_blue->mutable_position ()->set_x (u_iter->position.x ());
        unit_for_blue->mutable_position ()->set_y (u_iter->position.y ());
        unit_for_blue->set_health (u_iter->hp);
        unit_for_blue->set_orientation (u_iter->orientation);
        unit_for_blue->set_id (u_iter.key ());
    }

    for (QHash<quint32, Missile>::const_iterator m_iter = match_state->missilesRef ().cbegin (); m_iter != match_state->missilesRef ().cend (); m_iter++) {
        RTS::Missile* missile_for_red = response_for_red->add_missiles ();
        RTS::Missile* missile_for_blue = response_for_blue->add_missiles ();
        switch (m_iter->type) {
        case Missile::Type::Pestilence: {
            missile_for_red->set_type (RTS::MissileType::MISSILE_PESTILENCE);
            missile_for_blue->set_type (RTS::MissileType::MISSILE_PESTILENCE);
        } break;
        case Missile::Type::Rocket: {
            missile_for_blue->set_type (RTS::MissileType::MISSILE_ROCKET);
            missile_for_red->set_type (RTS::MissileType::MISSILE_ROCKET);
        } break;
        }
        missile_for_blue->mutable_position ()->set_x (m_iter->position.x ());
        missile_for_blue->mutable_position ()->set_y (m_iter->position.y ());

        if (m_iter->target_unit.has_value ()) {
            missile_for_blue->mutable_target_unit ()->set_id (*(m_iter->target_unit));
            missile_for_red->mutable_target_unit ()->set_id (*(m_iter->target_unit));
        }
        missile_for_blue->mutable_target_position ()->set_x (m_iter->target_position.x ());
        missile_for_blue->mutable_target_position ()->set_y (m_iter->target_position.y ());
        missile_for_red->mutable_target_position ()->set_x (m_iter->target_position.x ());
        missile_for_red->mutable_target_position ()->set_y (m_iter->target_position.y ());

        missile_for_red->mutable_position ()->set_x (m_iter->position.x ());
        missile_for_red->mutable_position ()->set_y (m_iter->position.y ());

        missile_for_blue->set_id (m_iter.key ());
        missile_for_red->set_id (m_iter.key ());

        if (m_iter->sender_team == Unit::Team::Red) {
            missile_for_red->set_team (RTS::Team::TEAM_RED);
            missile_for_blue->set_team (RTS::Team::TEAM_RED);
        }
        if (m_iter->sender_team == Unit::Team::Blue) {
            missile_for_red->set_team (RTS::Team::TEAM_BLUE);
            missile_for_blue->set_team (RTS::Team::TEAM_BLUE);
        }
    }

    emit sendResponseRoom (response_for_red_oneof, red_team, {});
    emit sendResponseRoom (response_for_blue_oneof, blue_team, {});
}

void Room::setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code)
{
    error->set_message (error_message);
    error->set_code (error_code);
}

void Room::init_matchstate ()
{
    match_state.reset (new MatchState (false));
    {
        /*match_state->createUnit (Unit::Type::Goon, Unit::Team::Red, QPointF (-15, -7), 0);
        match_state->createUnit (Unit::Type::Contaminator, Unit::Team::Red, QPointF (1, 3), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Red, QPointF (8, 3), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Red, QPointF (8, 6), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Red, QPointF (8, 9), 0);
        match_state->createUnit (Unit::Type::Seal, Unit::Team::Blue, QPointF (10, 5), 0);
        match_state->createUnit (Unit::Type::Crusader, Unit::Team::Blue, QPointF (-20, -8), 0);*/
    }
}
void Room::emitStatsUpdated ()
{
    quint32 ready_player_count = 0;
    for (const QSharedPointer<Session>& session: players) {
        if (session->ready)
            ++ready_player_count;
    }
    emit statsUpdated (players.count (), ready_player_count, spectators.count ());
}
void Room::receiveRequestHandlerRoom (const RTS::Request& request_oneof, QSharedPointer<Session> session, quint64 request_id)
{
    switch (request_oneof.message_case ()) {
    case RTS::Request::MessageCase::kSelectRole: {
        const RTS::SelectRoleRequest& request = request_oneof.select_role ();

        if (request.role () == RTS::Role::ROLE_PLAYER) {
            if (players.size () >= 2) {
                RTS::Response response_oneof;
                RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
                setError (response->mutable_error (), "Too many players in room", RTS::ERROR_CODE_TOO_MANY_PLAYERS_IN_ROOM);
                emit sendResponseRoom (response_oneof, session, request_id);
                return;
            }
            session->current_role = RTS::Role::ROLE_PLAYER;
            players.append (session);
            emitStatsUpdated ();

            RTS::Response response_oneof;
            RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
            response->mutable_success ();
            emit sendResponseRoom (response_oneof, session, request_id);
        } else if (request.role () == RTS::Role::ROLE_SPECTATOR) {
            RTS::Response response_oneof;
            RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
            setError (response->mutable_error (), "Spectatorship not implemented", RTS::ERROR_CODE_NOT_IMPLEMENTED);
            emit sendResponseRoom (response_oneof, session, request_id);
        } else {
            RTS::Response response_oneof;
            RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
            setError (response->mutable_error (), "Invalid role specified", RTS::ERROR_CODE_MALFORMED_MESSAGE);
            emit sendResponseRoom (response_oneof, session, request_id);
        }

    } break;
    case RTS::Request::MessageCase::kReady: {
        session->ready = true;
        emitStatsUpdated ();

        RTS::Response response_oneof;
        RTS::ReadyResponse* response = response_oneof.mutable_ready ();
        response->mutable_success ();
        emit sendResponseRoom (response_oneof, session, request_id);

        if (players.size () == 2 && players[0]->ready && players[1]->ready) {
            red_team = players[0];
            blue_team = players[1];

            red_team->current_team = Unit::Team::Red;
            blue_team->current_team = Unit::Team::Blue;

            {
                RTS::Response response_oneof;
                RTS::MatchPreparedResponse* response = response_oneof.mutable_match_prepared ();
                response->set_team (RTS::Team::TEAM_RED);
                emit sendResponseRoom (response_oneof, red_team, request_id);
            }
            {
                RTS::Response response_oneof;
                RTS::MatchPreparedResponse* response = response_oneof.mutable_match_prepared ();
                response->set_team (RTS::Team::TEAM_BLUE);
                emit sendResponseRoom (response_oneof, blue_team, request_id);
            }

            QTimer::singleShot (5000, this, &Room::readyHandler);
        }
    } break;
    case RTS::Request::MessageCase::kUnitCreate: {
        const RTS::UnitCreateRequest& request = request_oneof.unit_create ();
        Unit::Team team = *session->current_team;
        Unit::Type type = Unit::Type::Goon; // TODO: Handle wrong value properly
        switch (request.unit_type ()) {
        case RTS::UnitType::UNIT_TYPE_CRUSADER: {
            type = Unit::Type::Crusader;
        } break;
        case RTS::UnitType::UNIT_TYPE_SEAL: {
            type = Unit::Type::Seal;
        } break;
        case RTS::UnitType::UNIT_TYPE_GOON: {
            type = Unit::Type::Goon;
        } break;
        case RTS::UnitType::UNIT_TYPE_BEETLE: {
            type = Unit::Type::Beetle;
        } break;
        case RTS::UnitType::UNIT_TYPE_CONTAMINATOR: {
            type = Unit::Type::Contaminator;
        } break;
        }
        QHash<quint32, Unit>::iterator unit = match_state->createUnit (type, team, QPointF (request.position ().x (), request.position ().y ()), 0);
        if (*session->current_team == Unit::Team::Red) {
            red_client_to_server[request.id ()] = unit.key ();
        } else if (*session->current_team == Unit::Team::Blue) {
            blue_client_to_server[request.id ()] = unit.key ();
        }
    } break;
    case RTS::Request::MessageCase::kUnitAction: {
        const RTS::UnitActionRequest& request = request_oneof.unit_action ();
        const RTS::UnitAction& action = request.action ();
        quint32 id = 0;
        if (session->current_team == Unit::Team::Red) {
            id = red_client_to_server[request.unit_id ()];
        } else if (session->current_team == Unit::Team::Blue) {
            id = blue_client_to_server[request.unit_id ()];
        }
        switch (action.action_case ()) {
        case RTS::UnitAction::ActionCase::kMove: {
            if (action.move ().has_position ()) {
                match_state->setAction (request.unit_id (), MoveAction (QPointF (request.action ().move ().position ().position ().x (),
                                                                                 request.action ().move ().position ().position ().y ())));
            } else if (action.move ().has_unit ()) {
                match_state->setAction (request.unit_id (), MoveAction (action.move ().unit ().id ()));
            } else {
                RTS::Response response_oneof;
                RTS::ErrorResponse* response = response_oneof.mutable_error ();
                setError (response->mutable_error (), "Malformed message", RTS::ERROR_CODE_MALFORMED_MESSAGE);
                emit sendResponseRoom (response_oneof, session, request_id);
            }

        } break;
        case RTS::UnitAction::ActionCase::kAttack: {
            if (action.attack ().has_position ()) {
                match_state->setAction (request.unit_id (), AttackAction (QPointF (request.action ().attack ().position ().position ().x (),
                                                                                   request.action ().attack ().position ().position ().y ())));
            } else if (action.attack ().has_unit ()) {
                match_state->setAction (request.unit_id (), AttackAction (action.attack ().unit ().id ()));
            } else {
                RTS::Response response_oneof;
                RTS::ErrorResponse* response = response_oneof.mutable_error ();
                setError (response->mutable_error (), "Malformed message", RTS::ERROR_CODE_MALFORMED_MESSAGE);
                emit sendResponseRoom (response_oneof, session, request_id);
            }

        } break;
        case RTS::UnitAction::ActionCase::kCast: {
            CastAction::Type type;
            type = CastAction::Type::Unknown;
            switch (action.cast ().type ()) {
            case (RTS::CastType::PESTILENCE): {
                type = CastAction::Type::Pestilence;
            } break;
            case (RTS::CastType::SPAWN_BEETLE): {
                type = CastAction::Type::SpawnBeetle;
            } break;
            }
            CastAction cast = CastAction (type, QPointF (action.cast ().position ().position ().x (), action.cast ().position ().position ().y ()));
            match_state->setAction (request.unit_id (), cast);
        } break;
        case RTS::UnitAction::ActionCase::kStop: {
            StopAction stop = StopAction ();
            if (action.stop ().has_target ()) {
                stop.current_target = action.stop ().target ().id ();
            } else {
                stop.current_target.reset ();
            }
            match_state->setAction (request.unit_id (), stop);
        } break;
        }
        // match_state->select(request.unit_id(), false);
        // match_state->move (QPointF(request.action().move().position().position().x(), request.action().move().position().position().y()));

    } break;
    default: {
        RTS::Response response_oneof;
        RTS::ErrorResponse* response = response_oneof.mutable_error ();
        setError (response->mutable_error (), "Unknown message from client", RTS::ERROR_CODE_MALFORMED_MESSAGE);
        emit sendResponseRoom (response_oneof, session, request_id);
    }
    }
}

void Room::readyHandler ()
{
    init_matchstate ();
    timer->start (kTickDurationMs);
    RTS::Response response_oneof;
    RTS::MatchStartResponse* response = response_oneof.mutable_match_start ();
    (void) response;
    emit sendResponseRoom (response_oneof, red_team, {});
    emit sendResponseRoom (response_oneof, blue_team, {});
}
