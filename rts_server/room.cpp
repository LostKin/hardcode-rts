#include "room.h"

#include "serialize.h"

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
    RTS::Response response_oneof;
    RTSN::Serialize::matchState (&*match_state, response_oneof, red_unit_id_client_to_server_map, blue_unit_id_client_to_server_map);

    emit sendResponseRoom (response_oneof, red_team, {});
    emit sendResponseRoom (response_oneof, blue_team, {});

    match_state->tick ();
}
void Room::setError (RTS::Error* error, const std::string& error_message, RTS::ErrorCode error_code)
{
    error->set_message (error_message);
    error->set_code (error_code);
}
void Room::init_matchstate ()
{
    match_state.reset (new MatchState ());
}
void Room::emitStatsUpdated ()
{
    uint32_t ready_player_count = 0;
    for (const std::shared_ptr<Session>& session: players) {
        if (session->ready)
            ++ready_player_count;
    }
    emit statsUpdated (players.size (), ready_player_count, spectators.size ());
}
void Room::receiveRequestHandlerRoom (const RTS::Request& request_oneof, std::shared_ptr<Session> session, uint64_t request_id)
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
            players.push_back (session);
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
