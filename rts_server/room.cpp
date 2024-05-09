#include "room.h"

#include <QThread>
#include <QUdpSocket>
#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QTimer>


static constexpr uint32_t kTickDurationMs = 20;


Room::Room (QObject* parent)
    : QObject (parent)
{
    timer.reset (new QTimer (this));
    connect (&*timer, &QTimer::timeout, this, &Room::tick);
}

void Room::tick ()
{
    uint32_t tick_no = match_state->get_tick_no ();
    RTS::Response response_for_red_oneof;
    RTS::Response response_for_blue_oneof;
    RTS::MatchStateResponse* response_for_red = response_for_red_oneof.mutable_match_state ();
    RTS::MatchStateResponse* response_for_blue = response_for_blue_oneof.mutable_match_state ();
    response_for_red->set_tick (tick_no);
    response_for_blue->set_tick (tick_no);

    auto* red_units = response_for_red->mutable_units ();
    auto* blue_units = response_for_blue->mutable_units ();
    for (std::map<uint32_t, Unit>::const_iterator it = match_state->unitsRef ().cbegin (); it != match_state->unitsRef ().cend (); it++) {
        uint32_t id = it->first;
        const Unit& unit = it->second;

        if (!fillUnit (id, unit, *red_units->Add ()))
            red_units->RemoveLast ();
        if (!fillUnit (id, unit, *blue_units->Add ()))
            blue_units->RemoveLast ();
    }

    auto* red_corpses = response_for_red->mutable_corpses ();
    auto* blue_corpses = response_for_blue->mutable_corpses ();
    for (std::map<uint32_t, Corpse>::const_iterator it = match_state->corpsesRef ().cbegin (); it != match_state->corpsesRef ().cend (); it++) {
        uint32_t id = it->first;
        const Corpse& corpse = it->second;

        if (!fillCorpse (id, corpse, *red_corpses->Add ()))
            red_corpses->RemoveLast ();
        if (!fillCorpse (id, corpse, *blue_corpses->Add ()))
            blue_corpses->RemoveLast ();
    }

    auto* red_missiles = response_for_red->mutable_missiles ();
    auto* blue_missiles = response_for_blue->mutable_missiles ();
    for (std::map<uint32_t, Missile>::const_iterator it = match_state->missilesRef ().cbegin (); it != match_state->missilesRef ().cend (); it++) {
        uint32_t id = it->first;
        const Missile& missile = it->second;

        if (!fillMissile (id, missile, *red_missiles->Add ()))
            red_missiles->RemoveLast ();
        if (!fillMissile (id, missile, *blue_missiles->Add ()))
            blue_missiles->RemoveLast ();
    }

    emit sendResponseRoom (response_for_red_oneof, red_team, {});
    emit sendResponseRoom (response_for_blue_oneof, blue_team, {});

    match_state->tick ();
}
bool Room::fillUnit (uint32_t id, const Unit& unit, RTS::Unit& m_unit)
{
    RTS::Team m_team;
    QMap<uint32_t, uint32_t>* unit_id_client_to_server_map;
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
        QMap<uint32_t, uint32_t>::const_iterator it = unit_id_client_to_server_map->find (id);
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
        fillStopAction (std::get<StopAction> (unit.action), m_unit.mutable_current_action ()->mutable_stop ());
    } else if (std::holds_alternative<MoveAction> (unit.action)) {
        fillMoveAction (std::get<MoveAction> (unit.action), m_unit.mutable_current_action ()->mutable_move ());
    } else if (std::holds_alternative<AttackAction> (unit.action)) {
        fillAttackAction (std::get<AttackAction> (unit.action), m_unit.mutable_current_action ()->mutable_attack ());
    } else if (std::holds_alternative<CastAction> (unit.action)) {
        if (!fillCastAction (std::get<CastAction> (unit.action), m_unit.mutable_current_action ()->mutable_cast ()))
            return false;
    } else if (std::holds_alternative<PerformingAttackAction> (unit.action)) {
        if (!fillPerformingAttackAction (std::get<PerformingAttackAction> (unit.action), m_unit.mutable_current_action ()->mutable_performing_attack ()))
            return false;
    } else if (std::holds_alternative<PerformingCastAction> (unit.action)) {
        if (!fillPerformingCastAction (std::get<PerformingCastAction> (unit.action), m_unit.mutable_current_action ()->mutable_performing_cast ()))
            return false;
    } else {
        return false;
    }
    m_unit.set_attack_cooldown_left_ticks (unit.attack_cooldown_left_ticks);
    m_unit.set_cast_cooldown_left_ticks (unit.cast_cooldown_left_ticks);

    m_unit.mutable_position ()->set_x (unit.position.x ());
    m_unit.mutable_position ()->set_y (unit.position.y ());
    m_unit.set_health (unit.hp);
    m_unit.set_orientation (unit.orientation);
    m_unit.set_id (id);

    return true;
}
bool Room::fillCorpse (uint32_t id, const Corpse& corpse, RTS::Corpse& m_corpse)
{
    m_corpse.set_decay_remaining_ticks (corpse.decay_remaining_ticks);
    return Room::fillUnit (id, corpse.unit, *m_corpse.mutable_unit ());
}
bool Room::fillMissile (uint32_t id, const Missile& missile, RTS::Missile& m_missile)
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
void Room::fillStopAction (const StopAction& stop_action, RTS::StopAction* m_stop_action)
{
    if (stop_action.current_target.has_value ())
        m_stop_action->mutable_target ()->set_id (stop_action.current_target.value ());
}
void Room::fillMoveAction (const MoveAction& move_action, RTS::MoveAction* m_move_action)
{
    if (std::holds_alternative<Position> (move_action.target)) {
        const Position& position = std::get<Position> (move_action.target);
        m_move_action->mutable_position ()->mutable_position ()->set_x (position.x ());
        m_move_action->mutable_position ()->mutable_position ()->set_y (position.y ());
    } else {
        uint32_t id = std::get<uint32_t> (move_action.target);
        m_move_action->mutable_unit ()->set_id (id);
    }
}
void Room::fillAttackAction (const AttackAction& attack_action, RTS::AttackAction* m_attack_action)
{
    if (std::holds_alternative<Position> (attack_action.target)) {
        const Position& position = std::get<Position> (attack_action.target);
        m_attack_action->mutable_position ()->mutable_position ()->set_x (position.x ());
        m_attack_action->mutable_position ()->mutable_position ()->set_y (position.y ());
    } else {
        uint32_t id = std::get<uint32_t> (attack_action.target);
        m_attack_action->mutable_unit ()->set_id (id);
    }
}
bool Room::fillCastAction (const CastAction& cast_action, RTS::CastAction* m_cast_action)
{
    RTS::CastType type;
    switch (cast_action.type) {
    case CastAction::Type::Pestilence:
        type = RTS::CastType::CAST_TYPE_PESTILENCE;
        break;
    case CastAction::Type::SpawnBeetle:
        type = RTS::CastType::CAST_TYPE_SPAWN_BEETLE;
        break;
    default:
        return false;
    }
    const Position& position = cast_action.target;
    m_cast_action->mutable_position ()->mutable_position ()->set_x (position.x ());
    m_cast_action->mutable_position ()->mutable_position ()->set_y (position.y ());
    m_cast_action->set_type (type);
    return true;
}
bool Room::fillPerformingAttackAction (const PerformingAttackAction& performing_attack_action, RTS::PerformingAttackAction* m_performing_attack_action)
{
    if (std::holds_alternative<StopAction> (performing_attack_action.next_action)) {
        fillStopAction (std::get<StopAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_stop ());
    } else if (std::holds_alternative<MoveAction> (performing_attack_action.next_action)) {
        fillMoveAction (std::get<MoveAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_move ());
    } else if (std::holds_alternative<AttackAction> (performing_attack_action.next_action)) {
        fillAttackAction (std::get<AttackAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_attack ());
    } else if (std::holds_alternative<CastAction> (performing_attack_action.next_action)) {
        if (!fillCastAction (std::get<CastAction> (performing_attack_action.next_action), m_performing_attack_action->mutable_cast ()))
            return false;
    } else {
        return false;
    }
    m_performing_attack_action->set_remaining_ticks (performing_attack_action.remaining_ticks);
    return true;
}
bool Room::fillPerformingCastAction (const PerformingCastAction& performing_cast_action, RTS::PerformingCastAction* m_performing_cast_action)
{
    if (std::holds_alternative<StopAction> (performing_cast_action.next_action)) {
        fillStopAction (std::get<StopAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_stop ());
    } else if (std::holds_alternative<MoveAction> (performing_cast_action.next_action)) {
        fillMoveAction (std::get<MoveAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_move ());
    } else if (std::holds_alternative<AttackAction> (performing_cast_action.next_action)) {
        fillAttackAction (std::get<AttackAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_attack ());
    } else if (std::holds_alternative<CastAction> (performing_cast_action.next_action)) {
        if (!fillCastAction (std::get<CastAction> (performing_cast_action.next_action), m_performing_cast_action->mutable_cast ()))
            return false;
    } else {
        return false;
    }
    RTS::CastType cast_type;
    switch (performing_cast_action.cast_type) {
    case CastAction::Type::Pestilence:
        cast_type = RTS::CastType::CAST_TYPE_PESTILENCE;
        break;
    case CastAction::Type::SpawnBeetle:
        cast_type = RTS::CastType::CAST_TYPE_SPAWN_BEETLE;
        break;
    default:
        return false;
    }
    m_performing_cast_action->set_cast_type (cast_type);
    m_performing_cast_action->set_remaining_ticks (performing_cast_action.remaining_ticks);
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
}
void Room::emitStatsUpdated ()
{
    uint32_t ready_player_count = 0;
    for (const QSharedPointer<Session>& session: players) {
        if (session->ready)
            ++ready_player_count;
    }
    emit statsUpdated (players.count (), ready_player_count, spectators.count ());
}
void Room::receiveRequestHandlerRoom (const RTS::Request& request_oneof, QSharedPointer<Session> session, uint64_t request_id)
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
        Unit::Type type;
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
        default: {
            RTS::Response response_oneof;
            RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
            setError (response->mutable_error (), "Invalid role specified", RTS::ERROR_CODE_MALFORMED_MESSAGE);
            emit sendResponseRoom (response_oneof, session, request_id);
        } return;
        }
        Unit::Team team = *session->current_team;
        if (!request.has_position ()) {
            RTS::Response response_oneof;
            RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
            setError (response->mutable_error (), "No position specified", RTS::ERROR_CODE_MALFORMED_MESSAGE);
            emit sendResponseRoom (response_oneof, session, request_id);
        }
        const RTS::Vector2D& position = request.position ();
        std::map<uint32_t, Unit>::iterator unit = match_state->createUnit (type, team, Position (position.x (), position.y ()), 0);
        if (*session->current_team == Unit::Team::Red) {
            red_unit_id_client_to_server_map[request.id ()] = unit->first;
        } else if (*session->current_team == Unit::Team::Blue) {
            blue_unit_id_client_to_server_map[request.id ()] = unit->first;
        } else {
            RTS::Response response_oneof;
            RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
            setError (response->mutable_error (), "Unexpected team specified", RTS::ERROR_CODE_MALFORMED_MESSAGE);
            emit sendResponseRoom (response_oneof, session, request_id);
        }
    } break;
    case RTS::Request::MessageCase::kUnitAction: {
        const RTS::UnitActionRequest& request = request_oneof.unit_action ();
        const RTS::UnitAction& action = request.action ();
        uint32_t id;
        if (session->current_team == Unit::Team::Red) {
            id = red_unit_id_client_to_server_map[request.unit_id ()];
        } else if (session->current_team == Unit::Team::Blue) {
            id = blue_unit_id_client_to_server_map[request.unit_id ()];
        } else {
            RTS::Response response_oneof;
            RTS::ErrorResponse* response = response_oneof.mutable_error ();
            setError (response->mutable_error (), "Malformed message", RTS::ERROR_CODE_MALFORMED_MESSAGE);
            emit sendResponseRoom (response_oneof, session, request_id);
            return;
        }
        switch (action.action_case ()) {
        case RTS::UnitAction::ActionCase::kMove: {
            if (action.move ().has_position ()) {
                match_state->setUnitAction (request.unit_id (), MoveAction (Position (action.move ().position ().position ().x (),
                                                                                     action.move ().position ().position ().y ())));
            } else if (action.move ().has_unit ()) {
                match_state->setUnitAction (request.unit_id (), MoveAction (action.move ().unit ().id ()));
            } else {
                RTS::Response response_oneof;
                RTS::ErrorResponse* response = response_oneof.mutable_error ();
                setError (response->mutable_error (), "Malformed message", RTS::ERROR_CODE_MALFORMED_MESSAGE);
                emit sendResponseRoom (response_oneof, session, request_id);
            }
        } break;
        case RTS::UnitAction::ActionCase::kAttack: {
            const RTS::AttackAction& attack = action.attack ();
            if (attack.has_position ()) {
                const RTS::Vector2D& target_position = attack.position ().position ();
                match_state->setUnitAction (request.unit_id (), AttackAction (Position (target_position.x (), target_position.y ())));
            } else if (attack.has_unit ()) {
                match_state->setUnitAction (request.unit_id (), AttackAction (attack.unit ().id ()));
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
            default: {
                RTS::Response response_oneof;
                RTS::ErrorResponse* response = response_oneof.mutable_error ();
                setError (response->mutable_error (), "Malformed message: invalid cast type", RTS::ERROR_CODE_MALFORMED_MESSAGE);
                emit sendResponseRoom (response_oneof, session, request_id);
            } return;
            }
            CastAction cast = CastAction (type, Position (action.cast ().position ().position ().x (), action.cast ().position ().position ().y ()));
            match_state->setUnitAction (request.unit_id (), cast);
        } break;
        case RTS::UnitAction::ActionCase::kStop: {
            StopAction stop = StopAction ();
            if (action.stop ().has_target ()) {
                stop.current_target = action.stop ().target ().id ();
            } else {
                stop.current_target.reset ();
            }
            match_state->setUnitAction (request.unit_id (), stop);
        } break;
        default: {
            RTS::Response response_oneof;
            RTS::SelectRoleResponse* response = response_oneof.mutable_select_role ();
            setError (response->mutable_error (), "Invalid unit action specified", RTS::ERROR_CODE_MALFORMED_MESSAGE);
            emit sendResponseRoom (response_oneof, session, request_id);
        }
        }
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
