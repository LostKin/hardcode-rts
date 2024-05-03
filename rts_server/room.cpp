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

    auto* red_units = response_for_red->mutable_units ();
    auto* blue_units = response_for_blue->mutable_units ();
    for (QHash<quint32, Unit>::const_iterator it = match_state->unitsRef ().cbegin (); it != match_state->unitsRef ().cend (); it++) {
        quint32 id = it.key ();
        const Unit& unit = *it;

        if (!fillUnit (id, unit, *red_units->Add ()))
            red_units->RemoveLast ();
        if (!fillUnit (id, unit, *blue_units->Add ()))
            blue_units->RemoveLast ();
    }

    auto* red_corpses = response_for_red->mutable_corpses ();
    auto* blue_corpses = response_for_blue->mutable_corpses ();
    for (QHash<quint32, Corpse>::const_iterator it = match_state->corpsesRef ().cbegin (); it != match_state->corpsesRef ().cend (); it++) {
        quint32 id = it.key ();
        const Corpse& corpse = *it;

        if (!fillCorpse (id, corpse, *red_corpses->Add ()))
            red_corpses->RemoveLast ();
        if (!fillCorpse (id, corpse, *blue_corpses->Add ()))
            blue_corpses->RemoveLast ();
    }

    auto* red_missiles = response_for_red->mutable_missiles ();
    auto* blue_missiles = response_for_blue->mutable_missiles ();
    for (QHash<quint32, Missile>::const_iterator it = match_state->missilesRef ().cbegin (); it != match_state->missilesRef ().cend (); it++) {
        quint32 id = it.key ();
        const Missile& missile = *it;

        if (!fillMissile (id, missile, *red_missiles->Add ()))
            red_missiles->RemoveLast ();
        if (!fillMissile (id, missile, *blue_missiles->Add ()))
            blue_missiles->RemoveLast ();
    }

    emit sendResponseRoom (response_for_red_oneof, red_team, {});
    emit sendResponseRoom (response_for_blue_oneof, blue_team, {});
}
bool Room::fillUnit (quint32 id, const Unit& unit, RTS::Unit& m_unit)
{
    RTS::Team m_team;
    QMap<quint32, quint32>* unit_id_client_to_server_map;
    switch (unit.team) {
    case Unit::Team::Red: {
        m_team = RTS::Team::TEAM_RED;
        unit_id_client_to_server_map = &red_unit_id_client_to_server_map;
    } break;
    case Unit::Team::Blue: {
        m_team = RTS::Team::TEAM_BLUE;
        unit_id_client_to_server_map = &blue_unit_id_client_to_server_map;
    } break;
    default: {
    } return false;
    }
    m_unit.set_team (m_team);
    {
        QMap<quint32, quint32>::const_iterator it = unit_id_client_to_server_map->find (id);
        if (it != unit_id_client_to_server_map->cend ())
            m_unit.mutable_client_id ()->set_id (it.key ());
    }
    switch (unit.type) {
    case Unit::Type::Seal:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_SEAL);
        break;
    case Unit::Type::Crusader:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_CRUSADER);
        break;
    case Unit::Type::Goon:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_GOON);
        break;
    case Unit::Type::Beetle:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_BEETLE);
        break;
    case Unit::Type::Contaminator:
        m_unit.set_type (RTS::UnitType::UNIT_TYPE_CONTAMINATOR);
        break;
    default:
        return false;
    }
    if (std::holds_alternative<StopAction> (unit.action)) {
        if (std::get<StopAction> (unit.action).current_target.has_value ())
            m_unit.mutable_current_action ()->mutable_stop ()->mutable_target ()->set_id (std::get<StopAction> (unit.action).current_target.value ());
        else
            m_unit.mutable_current_action ()->mutable_stop ();
    } else if (std::holds_alternative<MoveAction> (unit.action)) {
        if (std::holds_alternative<QPointF> (std::get<MoveAction> (unit.action).target)) {
            QPointF position = std::get<QPointF> (std::get<MoveAction> (unit.action).target);
            m_unit.mutable_current_action ()->mutable_move ()->mutable_position ()->mutable_position ()->set_x (position.x ());
            m_unit.mutable_current_action ()->mutable_move ()->mutable_position ()->mutable_position ()->set_y (position.y ());
        } else {
            quint32 id = std::get<quint32> (std::get<MoveAction> (unit.action).target);
            m_unit.mutable_current_action ()->mutable_move ()->mutable_unit ()->set_id (id);
        }
    } else if (std::holds_alternative<AttackAction> (unit.action)) {
        if (std::holds_alternative<QPointF> (std::get<AttackAction> (unit.action).target)) {
            QPointF position = std::get<QPointF> (std::get<AttackAction> (unit.action).target);
            m_unit.mutable_current_action ()->mutable_attack ()->mutable_position ()->mutable_position ()->set_x (position.x ());
            m_unit.mutable_current_action ()->mutable_attack ()->mutable_position ()->mutable_position ()->set_y (position.y ());
        } else {
            quint32 id = std::get<quint32> (std::get<AttackAction> (unit.action).target);
            m_unit.mutable_current_action ()->mutable_attack ()->mutable_unit ()->set_id (id);
        }
    } else if (std::holds_alternative<CastAction> (unit.action)) {
        RTS::CastType type;
        switch (std::get<CastAction> (unit.action).type) {
        case CastAction::Type::Pestilence:
            type = RTS::CastType::CAST_TYPE_PESTILENCE;
            break;
        case CastAction::Type::SpawnBeetle:
            type = RTS::CastType::CAST_TYPE_SPAWN_BEETLE;
            break;
        default:
            return false;
        }
        QPointF position = std::get<CastAction> (unit.action).target;
        m_unit.mutable_current_action ()->mutable_cast ()->mutable_position ()->mutable_position ()->set_x (position.x ());
        m_unit.mutable_current_action ()->mutable_cast ()->mutable_position ()->mutable_position ()->set_y (position.y ());
        m_unit.mutable_current_action ()->mutable_cast ()->set_type (type);
    }
    m_unit.set_attack_remaining_ticks (unit.attack_remaining_ticks);
    m_unit.set_cooldown (unit.cast_cooldown_left_ticks);

    m_unit.mutable_position ()->set_x (unit.position.x ());
    m_unit.mutable_position ()->set_y (unit.position.y ());
    m_unit.set_health (unit.hp);
    m_unit.set_orientation (unit.orientation);
    m_unit.set_id (id);

    return true;
}
bool Room::fillCorpse (quint32 id, const Corpse& corpse, RTS::Corpse& m_corpse)
{
    m_corpse.set_decay_remaining_ticks (corpse.decay_remaining_ticks);
    return Room::fillUnit (id, corpse.unit, *m_corpse.mutable_unit ());
}
bool Room::fillMissile (quint32 id, const Missile& missile, RTS::Missile& m_missile)
{
    switch (missile.type) {
    case Missile::Type::Pestilence:
        m_missile.set_type (RTS::MissileType::MISSILE_PESTILENCE);
        break;
    case Missile::Type::Rocket:
        m_missile.set_type (RTS::MissileType::MISSILE_ROCKET);
        break;
    default:
        return false;
    }

    if (missile.target_unit.has_value ())
        m_missile.mutable_target_unit ()->set_id (*(missile.target_unit));

    m_missile.mutable_target_position ()->set_x (missile.target_position.x ());
    m_missile.mutable_target_position ()->set_y (missile.target_position.y ());

    m_missile.mutable_position ()->set_x (missile.position.x ());
    m_missile.mutable_position ()->set_y (missile.position.y ());

    m_missile.set_id (id);

    switch (missile.sender_team) {
    case Unit::Team::Red:
        m_missile.set_team (RTS::Team::TEAM_RED);
        break;
    case Unit::Team::Blue:
        m_missile.set_team (RTS::Team::TEAM_BLUE);
        break;
    default:
        return false;
    }

    return true;
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
            red_unit_id_client_to_server_map[request.id ()] = unit.key ();
        } else if (*session->current_team == Unit::Team::Blue) {
            blue_unit_id_client_to_server_map[request.id ()] = unit.key ();
        }
    } break;
    case RTS::Request::MessageCase::kUnitAction: {
        const RTS::UnitActionRequest& request = request_oneof.unit_action ();
        const RTS::UnitAction& action = request.action ();
        quint32 id = 0;
        if (session->current_team == Unit::Team::Red) {
            id = red_unit_id_client_to_server_map[request.unit_id ()];
        } else if (session->current_team == Unit::Team::Blue) {
            id = blue_unit_id_client_to_server_map[request.unit_id ()];
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
            case (RTS::CastType::CAST_TYPE_PESTILENCE): {
                type = CastAction::Type::Pestilence;
            } break;
            case (RTS::CastType::CAST_TYPE_SPAWN_BEETLE): {
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
