#include "room.h"

#include <QThread>
#include <QUdpSocket>
#include <QDebug>
#include <QCoreApplication>
#include <QNetworkDatagram>
#include <QTimer>

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
    QVector<RTS::Response> responses_for_red, responses_for_blue;
    // RTS::Response response_oneof_for_red, response_oneof_for_blue;
    QHash<quint32, Unit>::const_iterator u_iter = match_state->unitsRef ().cbegin ();
    while (u_iter != match_state->unitsRef ().cend ()) {
        responses_for_red.push_back (RTS::Response ());
        responses_for_blue.push_back (RTS::Response ());
        RTS::MatchState* response_for_red = responses_for_red.rbegin ()->mutable_match_state ();
        RTS::MatchState* response_for_blue = responses_for_blue.rbegin ()->mutable_match_state ();
        for (int i = 0; i < 16; i++) {
            if (u_iter == match_state->unitsRef ().cend ()) {
                break;
            }
            RTS::Unit* unit_for_red = response_for_red->add_units ();
            RTS::Unit* unit_for_blue = response_for_blue->add_units ();
            if (u_iter->team == Unit::Team::Red) {
                unit_for_red->set_team (RTS::Team::RED);
                unit_for_blue->set_team (RTS::Team::RED);
                QMap<quint32, quint32>::const_iterator it = red_client_to_server.find (u_iter.key ());
                if (it != red_client_to_server.cend ()) {
                    unit_for_red->mutable_client_id ()->set_id (it.key ());
                }
            }
            if (u_iter->team == Unit::Team::Blue) {
                unit_for_red->set_team (RTS::Team::BLUE);
                unit_for_blue->set_team (RTS::Team::BLUE);
                QMap<quint32, quint32>::const_iterator it = blue_client_to_server.find (u_iter.key ());
                if (it != blue_client_to_server.cend ()) {
                    unit_for_blue->mutable_client_id ()->set_id (it.key ());
                }
            }
            switch (u_iter->type) {
            case Unit::Type::Seal: {
                unit_for_red->set_type (RTS::UnitType::SEAL);
                unit_for_blue->set_type (RTS::UnitType::SEAL);
            } break;
            case Unit::Type::Crusader: {
                unit_for_red->set_type (RTS::UnitType::CRUSADER);
                unit_for_blue->set_type (RTS::UnitType::CRUSADER);
            } break;
            case Unit::Type::Goon: {
                unit_for_red->set_type (RTS::UnitType::GOON);
                unit_for_blue->set_type (RTS::UnitType::GOON);
            } break;
            case Unit::Type::Beetle: {
                unit_for_red->set_type (RTS::UnitType::BEETLE);
                unit_for_blue->set_type (RTS::UnitType::BEETLE);
            } break;
            case Unit::Type::Contaminator: {
                unit_for_red->set_type (RTS::UnitType::CONTAMINATOR);
                unit_for_blue->set_type (RTS::UnitType::CONTAMINATOR);
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
            u_iter++;
        }
    }

    QHash<quint32, Missile>::const_iterator m_iter = match_state->missilesRef ().cbegin ();

    while (m_iter != match_state->missilesRef ().cend ()) {
        responses_for_red.push_back (RTS::Response ());
        responses_for_blue.push_back (RTS::Response ());
        RTS::MatchState* response_for_red = responses_for_red.rbegin ()->mutable_match_state ();
        RTS::MatchState* response_for_blue = responses_for_blue.rbegin ()->mutable_match_state ();
        for (int i = 0; i < 16; i++) {
            if (m_iter == match_state->missilesRef ().cend ()) {
                break;
            }
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
                missile_for_red->set_team (RTS::Team::RED);
                missile_for_blue->set_team (RTS::Team::RED);
            }
            if (m_iter->sender_team == Unit::Team::Blue) {
                missile_for_red->set_team (RTS::Team::BLUE);
                missile_for_blue->set_team (RTS::Team::BLUE);
            }
            m_iter++;
        }
    }
    // response->units;

    for (int i = 0; i < responses_for_red.size (); i++) {
        responses_for_red[i].mutable_match_state ()->set_fragment_no (i);
        responses_for_blue[i].mutable_match_state ()->set_fragment_no (i);
        responses_for_red[i].mutable_match_state ()->set_fragment_count (responses_for_red.size ());
        responses_for_blue[i].mutable_match_state ()->set_fragment_count (responses_for_blue.size ());
        responses_for_red[i].mutable_match_state ()->set_tick (tick_no);
        responses_for_blue[i].mutable_match_state ()->set_tick (tick_no);
        emit sendResponseRoom (responses_for_red[i], red_team);
        emit sendResponseRoom (responses_for_blue[i], blue_team);
    }

    // sampling = 1 - sampling;
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

void Room::receiveRequestHandlerRoom (RTS::Request request_oneof, QSharedPointer<Session> session)
{
    // qDebug() << "Room receive request started";
    switch (request_oneof.message_case ()) {
    case RTS::Request::MessageCase::kJoinTeam: {
        const RTS::JoinTeamRequest& request = request_oneof.join_team ();

        if (session->current_team.has_value ()) {
            // error - player already has a team
            RTS::Response response_oneof;
            RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
            response->mutable_request_token ()->set_value (request_oneof.join_team ().request_token ().value ());
            setError (response->mutable_error (), "Already joined a team", RTS::ALREADY_JOINED_TEAM);
            emit sendResponseRoom (response_oneof, session);
            return;
        }

        if (request.team () == RTS::Team::RED) {
            if (!red_team.isNull ()) {
                RTS::Response response_oneof;
                RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
                response->mutable_request_token ()->set_value (request_oneof.join_team ().request_token ().value ());
                setError (response->mutable_error (), "Team already taken", RTS::TEAM_ALREADY_TAKEN);
                emit sendResponseRoom (response_oneof, session);
                return;
            }
            session->current_team = RTS::Team::RED;
            red_team = session;
        }
        if (request.team () == RTS::Team::BLUE) {
            if (!blue_team.isNull ()) {
                RTS::Response response_oneof;
                RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
                response->mutable_request_token ()->set_value (request_oneof.join_team ().request_token ().value ());
                setError (response->mutable_error (), "Team already taken", RTS::TEAM_ALREADY_TAKEN);
                emit sendResponseRoom (response_oneof, session);
                return;
            }
            session->current_team = RTS::Team::BLUE;
            blue_team = session;
        }
        qDebug () << "Successfully joined team";
        // teams[request.team_id()] = session;
        RTS::Response response_oneof;
        RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
        response->mutable_request_token ()->set_value (request_oneof.join_team ().request_token ().value ());
        response->mutable_success ();
        emit sendResponseRoom (response_oneof, session);
        return;
    } break;
    case RTS::Request::MessageCase::kReady: {
        /*const RTS::ReadyRequest& request = request_oneof.ready ();
        if (teams[request.team_id()] == 0, ) {
            RTS::Response response_oneof;
            RTS::JoinTeamResponse* response = response_oneof.mutable_join_team ();
            response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
            setError(response->mutable_error(), "Team already taken", RTS::TEAM_ALREADY_TAKEN);
            emit sendResponseRoom(response_oneof, session);
            return;
        }
        RTS::Response response_oneof;
        RTS::ReadyResponse* response = response_oneof.mutable_join_team ();
        response->mutable_request_token ()->set_value (request_oneof.join_team().request_token().value());
        response->mutable_success ();
        emit sendResponseRoom(response_oneof, session);*/
        if (session->current_team == RTS::Team::RED) {
            qDebug () << "red ready";
        }

        if (session->current_team == RTS::Team::BLUE) {
            qDebug () << "blue ready";
        }
        session->ready = true;
        if (!red_team.isNull () && !blue_team.isNull () && red_team->ready && blue_team->ready) {
            qDebug () << "both ready";
            RTS::Response response_oneof;
            RTS::ReadyResponse* response = response_oneof.mutable_ready ();
            response->mutable_success ();
            emit sendResponseRoom (response_oneof, red_team);
            emit sendResponseRoom (response_oneof, blue_team);
            QTimer::singleShot (5000, this, &Room::readyHandler);
        }
        return;
    } break;
    case RTS::Request::MessageCase::kUnitCreate: {
        const RTS::UnitCreateRequest& request = request_oneof.unit_create ();
        Unit::Team team;
        team = Unit::Team::Red;
        if (session->current_team == RTS::Team::RED) {
            team = Unit::Team::Red;
        } else if (session->current_team == RTS::Team::BLUE) {
            team = Unit::Team::Blue;
        } else {
            RTS::Response response_oneof;
            RTS::ErrorResponse* response = response_oneof.mutable_error ();
            response->mutable_request_token ()->set_value (request_oneof.join_team ().request_token ().value ());
            setError (response->mutable_error (), "Malformed message", RTS::MALFORMED_MESSAGE);
            emit sendResponseRoom (response_oneof, session);
        }
        Unit::Type type = Unit::Type::Goon;
        switch (request.unit_type ()) {
        case RTS::UnitType::CRUSADER: {
            type = Unit::Type::Crusader;
        } break;
        case RTS::UnitType::SEAL: {
            type = Unit::Type::Seal;
        } break;
        case RTS::UnitType::GOON: {
            type = Unit::Type::Goon;
        } break;
        case RTS::UnitType::BEETLE: {
            type = Unit::Type::Beetle;
        } break;
        case RTS::UnitType::CONTAMINATOR: {
            type = Unit::Type::Contaminator;
        } break;
        }
        // qDebug() << "room.cpp" << request.position().x() << request.position().y();
        QHash<quint32, Unit>::iterator unit = match_state->createUnit (type, team, QPointF (request.position ().x (), request.position ().y ()), 0);
        if (session->current_team == RTS::Team::BLUE) {
            blue_client_to_server[request.id ()] = unit.key ();
        } else if (session->current_team == RTS::Team::RED) {
            red_client_to_server[request.id ()] = unit.key ();
        }

    } break;
    case RTS::Request::MessageCase::kUnitAction: {
        const RTS::UnitActionRequest& request = request_oneof.unit_action ();
        const RTS::UnitAction& action = request.action ();
        quint32 id = 0;
        if (session->current_team == RTS::Team::RED) {
            id = red_client_to_server[request.unit_id ()];
        } else if (session->current_team == RTS::Team::BLUE) {
            id = blue_client_to_server[request.unit_id ()];
        }
        switch (action.action_case ()) {
        case RTS::UnitAction::ActionCase::kMove: {
            if (action.move ().has_position ()) {
                match_state->setAction (request.unit_id (), MoveAction (QPointF (request.action ().move ().position ().position ().x (),
                                                                                 request.action ().move ().position ().position ().y ())));
            } else if (action.move ().has_unit ()) {
                // qDebug() << "Moving to unit";
                match_state->setAction (request.unit_id (), MoveAction (action.move ().unit ().id ()));
            } else {
                RTS::Response response_oneof;
                RTS::ErrorResponse* response = response_oneof.mutable_error ();
                response->mutable_request_token ()->set_value (request_oneof.join_team ().request_token ().value ());
                setError (response->mutable_error (), "Malformed message", RTS::MALFORMED_MESSAGE);
                emit sendResponseRoom (response_oneof, session);
            }

        } break;
        case RTS::UnitAction::ActionCase::kAttack: {
            if (action.attack ().has_position ()) {
                match_state->setAction (request.unit_id (), AttackAction (QPointF (request.action ().attack ().position ().position ().x (),
                                                                                   request.action ().attack ().position ().position ().y ())));
                // qDebug() << "Attacking position";
            } else if (action.attack ().has_unit ()) {
                // qDebug() << "Moving to unit";
                match_state->setAction (request.unit_id (), AttackAction (action.attack ().unit ().id ()));
                // qDebug() << "Attacking unit" << acto;
            } else {
                RTS::Response response_oneof;
                RTS::ErrorResponse* response = response_oneof.mutable_error ();
                response->mutable_request_token ()->set_value (request_oneof.join_team ().request_token ().value ());
                setError (response->mutable_error (), "Malformed message", RTS::MALFORMED_MESSAGE);
                emit sendResponseRoom (response_oneof, session);
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
    }
}

void Room::readyHandler ()
{
    init_matchstate ();
    const quint32 TICK_LENGTH = 20;
    timer->start (TICK_LENGTH);
    qDebug () << "readyHandler started";
    RTS::Response response_oneof;
    RTS::MatchStartResponse* response = response_oneof.mutable_match_start ();
    emit sendResponseRoom (response_oneof, red_team);
    emit sendResponseRoom (response_oneof, blue_team);
    return;
}